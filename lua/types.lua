---@meta

---@alias CommitHash string
---@alias FilePath string
---@alias DiffText string
---@alias ParsedDiff table<string, table>

---@class GitApi
---@field git_diff_file fun(file: FilePath, from: CommitHash, to: CommitHash): DiffText
---@field git_diff_repo fun(from: CommitHash, to: CommitHash): DiffText
