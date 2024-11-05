local ERR_C_ERROR = 0x01

local scriptpath = debug.getinfo(2, "S").source:sub(2):match("(.*/)") or "./"
local so_dir = vim.fn.resolve(scriptpath .. "../..")
package.cpath = package.cpath .. ";" .. so_dir .. "/?.so"

local playback = require("playback")

local operations = {
  goto_position = function(pos, next_pos)
    local keys = {}
    if pos.line ~= next_pos.line then
      if pos.line > next_pos.line then
        local linediff = pos.line - next_pos.line
        if linediff > 1 then
          table.insert(keys, linediff .. "k")
        else
          table.insert(keys, "k")
        end
      else
        local linediff = next_pos.line - pos.line
        if linediff > 1 then
          table.insert(keys, linediff .. "j")
        else
          table.insert(keys, "j")
        end
      end
      if pos.column ~= 0 then table.insert(keys, "^") end
    end
    if next_pos.column > 1 then
      table.insert(keys, next_pos.column .. "l")
    elseif next_pos.column == 1 then
      table.insert(keys, "l")
    end
    return { keys = keys, cursor = next_pos }
  end,
  insert_word_after = function(current_pos, start_pos, src)
    local keys = { "i" }

    for i = 0, #src - 1 do
      local char = src:sub(i, i)
      if char == "\n" then
        table.insert(keys, "<CR>")
      else
        table.insert(keys, char)
      end
    end

    table.insert(keys, "<ESC>")

    local cursor = {
      line = current_pos.line,
      column = current_pos.column + #src,
    }
    return { keys = keys, cursor = cursor }
  end,
  insert_row_after = function(current_pos, start_pos, src)
    local keys = { "o" }

    for i = 0, #src - 1 do
      local char = src:sub(i, i)
      if char == "\n" then
        table.insert(keys, "<CR>")
      else
        table.insert(keys, char)
      end
    end

    table.insert(keys, "<CR>")

    local cursor = {
      line = current_pos.line + 1,
      column = #src,
    }
    return { keys = keys, cursor = cursor }
  end,
  move_rows = function(current_pos, start_pos, no_of_lines, move_amount)
    local keys = { "V" }
    if no_of_lines > 1 then
      if no_of_lines > 2 then
        table.insert(keys, (no_of_lines - 1) .. "j")
      else
        table.insert(keys, "j")
      end
    end
    if move_amount > 0 then
      table.insert(keys, string.rep("J", move_amount))
    else
      table.insert(keys, string.rep("K", -move_amount))
    end
    table.insert(keys, "<ESC>")

    local cursor = {
      line = current_pos.line + move_amount,
      column = current_pos.column,
    }

    return { keys = keys, cursor = cursor }
  end,
  delete_rows = function(current_pos, start_pos, no_of_lines)
    local keys = { "V" }
    if no_of_lines > 1 then
      if no_of_lines > 2 then
        table.insert(keys, (no_of_lines - 1) .. "j")
      else
        table.insert(keys, "j")
      end
    end
    table.insert(keys, "d")

    return { keys = keys, cursor = start_pos }
  end,
  delete_words = function(current_pos, start_pos, char_len)
    local keys = { "v" }
    if char_len > 1 then
      table.insert(keys, char_len .. "l")
    else
      table.insert("l")
    end
    table.insert(keys, "d")
    return { keys = keys, cursor = start_pos }
  end,
  concat_rows = function(current_pos, start_pos)
    local keys = { "J" }
    return { keys = keys, cursor = start_pos }
  end,
  split_rows = function(current_pos, start_pos)
    local keys = { "i", "<CR>", "<ESC>" }
    local cursor = {
      line = current_pos.line + 1,
      column = 0,
    }
    return { keys = keys, cursor = cursor }
  end,
}

local M = {}

M.getKeysFromPatch = function(patch)
  local success, keys = pcall(playback.getPatchKeys, { operations = operations }, patch)

  if not success then
    error("Patch application: " .. keys)
    return ERR_C_ERROR
  end

  return keys
end

M.processPatch = function(lhs, patch)
  local keys = M.getKeysFromPatch(patch)
  if keys == ERR_C_ERROR then return end

  vim.cmd("tabnew")
  vim.cmd("setlocal buftype=nofile bufhidden=wipe nobuflisted noswapfile nowrap")

  local lhs_lines = {}
  for line in lhs:gmatch("[^\r\n]+") do
    table.insert(lhs_lines, line)
  end
  vim.api.nvim_buf_set_lines(0, 0, -1, false, lhs_lines)

  local i = 1
  PrintNextKey = vim.schedule_wrap(function()
    local key = keys[i]
    if not key then return end
    print("Key:" .. key)
    key = vim.api.nvim_replace_termcodes(key, true, false, true)
    print("Key (replace termcode): " .. key)
    vim.api.nvim_feedkeys(key, "n", false)
    i = i + 1
    local t = vim.uv.new_timer()
    t:start(200, 0, PrintNextKey)
  end)

  local t = vim.uv.new_timer()
  t:start(200, 0, PrintNextKey)
  -- vim.defer_fn(PrintNextKey, 10)
end

local function processGitCmd(cmd)
  local f = assert(io.popen(cmd, "r"))
  local s = assert(f:read("*a"))
  f:close()
  return s
end

M.playbackFromCommit = function(lhs_commit, rhs_commit, file)
  print("LHS commit: " .. lhs_commit)
  print("RHS commit: " .. rhs_commit)
  print("File: " .. file)
  print(("git show " .. lhs_commit .. ":" .. file))
  print(("git show " .. rhs_commit .. ":" .. file))
  local lhs = processGitCmd("git show " .. lhs_commit .. ":" .. file)
  local rhs = processGitCmd("git show " .. rhs_commit .. ":" .. file)

  local diff = playback.generateDiff(lhs, rhs)
  local patch = playback.generatePatch(diff)
  M.processPatch(lhs, patch)
end

return M
