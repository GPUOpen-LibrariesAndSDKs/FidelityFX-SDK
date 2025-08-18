@echo off
set outputdir=%1
set outputdir=%outputdir:"=%

REM Get the latest commit hash (short format)
set "logCmd=git log -1 --pretty=format:"%%h""
for /f "tokens=1 delims= " %%i in ('git log -1 --oneline') do (set local_git_hash=%%i)

REM Get the current branch name
for /f %%i in ('git branch --show-current') do set local_git_commit_subject=%%i

REM Create the git_hash_branch.h file with the required content
(
    echo #define GIT_HASH "%local_git_hash%"
    echo #define GIT_COMMIT_SUBJECT "%local_git_commit_subject%"
) > "%outputdir%git_hash_branch.h"

echo "%outputdir%git_hash_branch.h" created successfully!
