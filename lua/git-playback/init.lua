local ERR_C_ERROR = 0x01

local import_ok, playback = pcall(require, "playback")
if not import_ok then
  error("Failed to import playback: " .. playback)
  return
end

local function insert_word(modekey)
  return function(current_pos, start_pos, src)
    local keys = { modekey }

    for i = 0, #src do
      local char = src:sub(i, i)
      if char == "\n" then
        table.insert(keys, "<CR>")
      else
        table.insert(keys, char)
      end
    end

    table.insert(keys, "<ESC>l")

    local cursor = {
      line = current_pos.line,
      column = current_pos.column + #src,
    }
    return { keys = keys, cursor = cursor }
  end
end

local function insert_row(modekey)
  return function(current_pos, start_pos, src)
    print("Inserting row with modekey " .. modekey)
    local keys = { modekey }

    for i = 0, #src - 1 do
      local char = src:sub(i, i)
      if char == "\n" then
        table.insert(keys, "<CR>")
      else
        table.insert(keys, char)
      end
    end

    -- table.insert(keys, "<CR>")
    table.insert(keys, "<ESC>")

    local cursor = {
      -- line = current_pos.line,
      line = modekey == "o" and current_pos.line + 1 or current_pos.line,
      column = #src - 1,
    }
    return { keys = keys, cursor = cursor }
  end
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
      elseif pos.line < next_pos.line then
        local linediff = next_pos.line - pos.line
        if linediff > 1 then
          table.insert(keys, linediff .. "j")
        else
          table.insert(keys, "j")
        end
      end
      if pos.column ~= 0 then
        table.insert(keys, "^")
        pos.column = 0
      end
    end
    if pos.column ~= next_pos.column then
      if next_pos.column == 0 then
        table.insert(keys, "^")
      else
        local coldiff = next_pos.column - pos.column
        if coldiff > 1 then
          table.insert(keys, coldiff .. "l")
        elseif coldiff == 1 then
          table.insert(keys, "l")
        elseif coldiff == -1 then
          table.insert(keys, "h")
        elseif coldiff < -1 then
          table.insert(keys, -coldiff .. "h")
        end
      end
    end
    return { keys = keys, cursor = next_pos }
  end,
  insert_word_before = insert_word("i"),
  insert_word_after = insert_word("a"),
  insert_row_before = insert_row("O"),
  insert_row_after = insert_row("o"),
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
    if char_len > 2 then
      table.insert(keys, (char_len - 1) .. "l")
    elseif char_len == 2 then
      table.insert(keys, "l")
    end
    table.insert(keys, "d")
    return { keys = keys, cursor = start_pos }
  end,
  concat_rows = function(current_pos, start_pos)
    local keys = { "J" }
    return { keys = keys, cursor = start_pos }
  end,
  split_rows = function(current_pos, start_pos)
    local keys = { "a", "<CR>", "<ESC>" }
    local cursor = {
      line = current_pos.line + 1,
      column = 0,
    }
    return { keys = keys, cursor = cursor }
  end,
  cut_words = function(_, start_pos, char_len)
    local keys = { "v" }
    if char_len > 2 then
      table.insert(keys, (char_len - 1) .. "l")
    elseif char_len == 2 then
      table.insert(keys, "l")
    end
    table.insert(keys, "x")
    return { keys = keys, cursor = start_pos }
  end,
  paste_words = function(_, start_pos)
    local keys = { "p" }
    return { keys = keys, cursor = start_pos }
  end,
}

local M = {}

M.playbackFromCommit = function(lhs_commit, rhs_commit, file)
  local ok
  local lhs, rhs
  local keys

  if not lhs_commit then
    ok, lhs = pcall(playback.showFileAtPath, file)
  else
    ok, lhs = pcall(playback.showFileAtRev, file, lhs_commit)
  end

  print("LHS: " .. lhs)

  if not ok then
    error("LHS File retrieval: " .. lhs)
    return ERR_C_ERROR
  end

  if not rhs_commit then
    ok, rhs = pcall(playback.showFileAtPath, file)
  else
    ok, rhs = pcall(playback.showFileAtRev, file, rhs_commit)
  end

  print("RHS: " .. rhs)

  if not ok then
    error("RHS File retrieval: " .. rhs)
    return ERR_C_ERROR
  end

  ok, keys = pcall(playback.getDiffKeys, { operations = operations }, lhs, rhs)

  if not ok then
    error("Patch application: " .. keys)
    return
  end

  print("Keys: " .. vim.inspect(keys))

  local filetype = vim.bo.filetype

  vim.cmd("tabnew")
  vim.cmd("setlocal buftype=nofile bufhidden=wipe nobuflisted noswapfile nowrap noautoindent filetype=" .. filetype)
  vim.cmd("set paste")

  local bufnr = vim.api.nvim_win_get_buf(0)

  local lhs_lines = {}
  for line in lhs:gmatch("[^\r\n]*\r?\n") do
    line = line:gsub("\n", "")
    table.insert(lhs_lines, line)
  end
  vim.api.nvim_buf_set_lines(0, 0, -1, false, lhs_lines)

  local speed = vim.g.playback_speed or 100

  local i = 1
  PrintNextKey = vim.schedule_wrap(function()
    if bufnr ~= vim.api.nvim_get_current_buf() then
      print("Aborted playback")
      vim.cmd("set nopaste")
      return
    end
    local key = keys[i]
    if not key then
      print("Playback complete")
      vim.cmd("set nopaste")
      return
    end
    vim.api.nvim_input(key)
    i = i + 1
    local t = vim.uv.new_timer()
    t:start(speed, 0, PrintNextKey)
  end)

  local t = vim.uv.new_timer()
  t:start(speed, 0, PrintNextKey)
end

return M
