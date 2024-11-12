local ERR_C_ERROR = 0x01

local scriptpath = debug.getinfo(2, "S").source:sub(2):match("(.*/)") or "./lua/git-playback"
local so_dir = vim.fn.resolve(scriptpath .. "../../../")
package.cpath = package.cpath .. ";" .. so_dir .. "/?.so"

local import_ok, playback = pcall(require, "playback")
if not import_ok then
  error("Failed to import playback: " .. playback)
  return
end

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

    for i = 0, #src do
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
    local keys = { "I" }

    for i = 0, #src - 1 do
      local char = src:sub(i, i)
      if char == "\n" then
        table.insert(keys, "<CR>")
      else
        table.insert(keys, char)
      end
    end

    table.insert(keys, "<CR>")
    table.insert(keys, "<ESC>")

    local cursor = {
      line = current_pos.line + 1,
      column = #src,
    }
    return { keys = keys, cursor = cursor }
  end,
  move_rows = function(current_pos, start_pos, no_of_lines, move_amount)
    local keys = { "V" }
    local abs_move_amount = math.abs(move_amount)
    if no_of_lines > 1 then
      if no_of_lines > 2 then
        table.insert(keys, (no_of_lines - 1) .. "j")
      else
        table.insert(keys, "j")
      end
    end
    if abs_move_amount > 5 then
      table.insert(keys, "d")
      table.insert(keys, (abs_move_amount + (move_amount > -1 and 0 or 1)) .. (move_amount > 0 and "j" or "k"))
      table.insert(keys, "p")
      table.insert(
        keys,
        (abs_move_amount + (move_amount > -1 and 1 or no_of_lines)) .. (move_amount > 0 and "k" or "j")
      )
    else
      if move_amount > 0 then
        for _ = 1, move_amount do
          table.insert(keys, "J")
        end
      else
        for _ = 1, move_amount do
          table.insert(keys, "K")
        end
      end
      table.insert(keys, "o")
      table.insert(keys, "<ESC>")
    end

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
  local ok, keys = pcall(playback.getPatchKeys, { operations = operations }, patch)

  if not ok then
    error("Patch application: " .. keys)
    return ERR_C_ERROR
  end

  return keys
end

M.processPatch = function(lhs, patch)
  local keys = M.getKeysFromPatch(patch)
  if keys == ERR_C_ERROR then return keys end

  local filetype = vim.bo.filetype

  vim.cmd("tabnew")
  vim.cmd("setlocal buftype=nofile bufhidden=wipe nobuflisted noswapfile nowrap")
  vim.cmd("setlocal filetype=" .. filetype)

  local bufnr = vim.api.nvim_win_get_buf(0)

  local lhs_lines = {}
  for line in lhs:gmatch("[^\r\n]+") do
    table.insert(lhs_lines, line)
  end
  vim.api.nvim_buf_set_lines(0, 0, -1, false, lhs_lines)

  local speed = vim.g.playback_speed or 100

  local i = 1
  PrintNextKey = vim.schedule_wrap(function()
    if bufnr ~= vim.api.nvim_get_current_buf() then
      print("Aborted playback")
      return
    end
    local key = keys[i]
    if not key then return end
    vim.api.nvim_input(key)
    i = i + 1
    local t = vim.uv.new_timer()
    t:start(speed, 0, PrintNextKey)
  end)

  local t = vim.uv.new_timer()
  t:start(speed, 0, PrintNextKey)
  -- vim.defer_fn(PrintNextKey, 10)
end

local function sysExec(cmd)
  local f = assert(io.popen(cmd, "r"))
  local s = assert(f:read("*a"))
  f:close()
  return s
end

M.playbackFromCommit = function(lhs_commit, rhs_commit, file)
  local ok

  local lhs, rhs

  -- if not lhs_commit then
  --   lhs = sysExec("cat " .. file)
  -- else
  --   lhs = sysExec("git show " .. lhs_commit .. ":" .. file)
  -- end
  --
  -- if not rhs_commit then
  --   rhs = sysExec("cat " .. file)
  -- else
  --   rhs = sysExec("git show " .. rhs_commit .. ":" .. file)
  -- end

  if not lhs_commit then
    ok, lhs = pcall(playback.showFileAtPath, file)
  else
    ok, lhs = pcall(playback.showFileAtRev, file, lhs_commit)
  end

  if not ok then
    error("LHS File retrieval: " .. lhs)
    return ERR_C_ERROR
  end

  if not rhs_commit then
    ok, rhs = pcall(playback.showFileAtPath, file)
  else
    ok, rhs = pcall(playback.showFileAtRev, file, rhs_commit)
  end

  if not ok then
    error("RHS File retrieval: " .. lhs)
    return ERR_C_ERROR
  end

  local diff, patch

  ok, diff = pcall(playback.generateDiff, lhs, rhs)
  if not ok then
    error("Diff generation: " .. diff)
    return ERR_C_ERROR
  end

  -- local diffdebug
  -- ok, diffdebug = pcall(playback.debugprintDiff, diff)
  -- if not ok then
  --   error("Diff debug print: " .. diffdebug)
  --   return ERR_C_ERROR
  -- end
  --
  -- print(diffdebug)

  ok, patch = pcall(playback.generatePatch, diff)
  if not ok then
    error("Patch generation: " .. patch)
    return ERR_C_ERROR
  end

  -- local patchdebug
  -- ok, patchdebug = pcall(playback.debugprintPatch, patch)
  -- if not ok then
  --   error("Diff debug print: " .. diffdebug)
  --   return ERR_C_ERROR
  -- end
  --
  -- -- print(patchdebug)

  M.processPatch(lhs, patch)
end

return M
