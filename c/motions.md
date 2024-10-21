
## Navigation
```lua

local modeshift = heuristic("modeshift", function(from, to, mode)
  if #from.lines > 0 and #to.lines > 0 then
    -- Moving a line
    if (mode == "n") then

    end
  end

  
  return 1;
end, { mode = "*" })

local jump_to_linechar = heuristic("jump_to_linechar", function(from, to, ctx)
  local line = from.lines
  repeat
    if line.index == from.pos.line
    line = line.next
  until line.next == nil
  local lineval = line:val()
  for i = 1, line:len() do
    local linechar = line.char + i
    if direction == "fwd" and linechar < from.pos.char then continue end
    if direction == "bwd" and linechar > from.pos.char then continue end


    if lineval[i] == char then
      if direction == "fwd" then
        local distance = i - from.pos.char
        if distance < 0 then
          return nil
        end
        return { cost = distance, modeshift = nil }
      elseif direction == "bwd" then
        local distance = from.pos.char - i
        if distance < 0 then
          return nil
        end
        return { cost = distance, modeshift = nil }
      end
    end
  end
end)

function select_linear(axis, amount)
  return function (from, to)
    local pos = from.pos
    local firstword = from.words
    local lastword = firstword
    repeat
      lastword = lastword.next
    until lastword.next == nil
    local cost
    local lwc = lastword.char + lastword.value:len() - 1
    if pos.char == firstword.char or pos.char == lwc then
      return lwc - firstword.char
    else
      return nil
    end
  end
end

function go_linear(axis, amount)
  return function(from, to)
    local pos = from.pos
    local target = from.words
    local lasttarget = nil

    repeat
      if axis == "x" then
        if amount > 0 and target.char > lasttarget.char then
          break
        elseif amount < 0 and target.char < lasttarget.char then
          break
        end
      elseif axis == "y" then
        if amount > 0 and target.line > lasttarget.line then
          break
        elseif amount < 0 and target.line < lasttarget.line then
          break
        end
      end

      lasttarget = target
    until ~(target = target.next)

    if axis == "x" then
      if amount > 0 then
        if target.char <= pos.char then
          return nil;
        else 
          cost = target.char - pos.char
        end
      elseif amount < 0 then
        local tc = target.char + target.value:len()
        if tc >= pos.char then
          return nil;
        else
          cost = pos.char - tc
        end
      end
    end

    return { cost = cost, modeshift = nil };
  end
end

function heuristic(from_line, from_col, from_char, line, col, char)
  return {
    mode("n", {
      ["h"] = go_one_left(),
    })
  }
end



-- [n,v]j
-- [n,v]k
-- [n,v]l
-- [n,v]w
-- [n,v]b
-- [n,v]e
-- [n,v]{
-- [n,v]}
-- [n,v]%
-- [n,v]<C-u>
-- [n,v]<C-d>
-- [n,v]<C-o>
-- [n,v]<C-i>
-- [v]o
-- [v]O
-- [n]gg
-- [n]G
-- [n]H
-- [n]M
-- [n]L
-- [n];
-- [n],
}
```

## Modeshift to normal
`
[i,v,V,vb,:]<ESC>
`

## Modeshift to insert
`
[n]i
[n]a
[n]I
[n]A
[n]o
[n]O
[n,v]c
`

## Modeshift to visual
`
[n,V,vb]v
`

## Modeshift to visual line
`
[n,v,vb]V
`

# Modeshift to visual block
`
[n,v,V]<C-v>
`

# Modeshift to command
`
[n,v,V,vb]:
`

# Deletions
`
[n,v,V,vb]x
[n,v,V,vb]d
`

# Repeat actions
`
[n].
`

# Undo/Redo
`
[n,v,V,vb]u
[n,v,V,vb]<C-r>
`
# Commands mode

`
[:]0<CR>
`


