Import("env")
from subprocess import check_output

git_commit = check_output(['git', 'rev-parse', '--verify', 'HEAD'], encoding='ascii').strip()
git_branch = check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], encoding='ascii').strip()

env.Append(CPPDEFINES=[
  ("GIT_COMMIT", git_commit),
  ("GIT_BRANCH", git_branch),
])
