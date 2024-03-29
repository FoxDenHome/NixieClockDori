#pragma once

#ifndef FW_VERSION
#ifdef GIT_COMMIT
#define FW_ADD_QUOTES_H(s) #s
#define FW_ADD_QUOTES(s) FW_ADD_QUOTES_H(s)
#define FW_VERSION FW_ADD_QUOTES(GIT_BRANCH) " " FW_ADD_QUOTES(GIT_COMMIT)
#else
#error "FW VERSION UNKNOWN"
#endif
#endif
