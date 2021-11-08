cd /D "%SKETCH_PATH%"

echo | set /p dummyName=#define GIT_COMMIT > buildinfo.h
git rev-parse --verify HEAD >> buildinfo.h

echo | set /p dummyName=#define GIT_BRANCH >> buildinfo.h
git rev-parse --abbrev-ref HEAD >> buildinfo.h

