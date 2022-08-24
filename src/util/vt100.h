#ifndef RAVEN_UTIL_VT100_H
#define RAVEN_UTIL_VT100_H

#ifdef RAVEN_USE_VT100

# define RAVEN_VT100_RESET   "\033[0m"
# define RAVEN_VT100_BOLD    "\033[1m"
# define RAVEN_VT100_ITALICS "\033[3m"
# define RAVEN_VT100_LINE    "\033[4m"
# define RAVEN_VT100_BLINK   "\033[5m"

# define RAVEN_VT100_FG_BLACK   "\033[30m"
# define RAVEN_VT100_FG_RED     "\033[31m"
# define RAVEN_VT100_FG_GREEN   "\033[32m"
# define RAVEN_VT100_FG_YELLOW  "\033[33m"
# define RAVEN_VT100_FG_BLUE    "\033[34m"
# define RAVEN_VT100_FG_MAGENTA "\033[35m"
# define RAVEN_VT100_FG_CYAN    "\033[36m"
# define RAVEN_VT100_FG_WHITE   "\033[37m"
# define RAVEN_VT100_FG_DEFAULT "\033[39m"

# define RAVEN_VT100_BG_BLACK   "\033[40m"
# define RAVEN_VT100_BG_RED     "\033[41m"
# define RAVEN_VT100_BG_GREEN   "\033[42m"
# define RAVEN_VT100_BG_YELLOW  "\033[43m"
# define RAVEN_VT100_BG_BLUE    "\033[44m"
# define RAVEN_VT100_BG_MAGENTA "\033[45m"
# define RAVEN_VT100_BG_CYAN    "\033[46m"
# define RAVEN_VT100_BG_WHITE   "\033[47m"
# define RAVEN_VT100_BG_DEFAULT "\033[49m"

#else

# define RAVEN_VT100_RESET   ""
# define RAVEN_VT100_BOLD    ""
# define RAVEN_VT100_ITALICS ""
# define RAVEN_VT100_LINE    ""
# define RAVEN_VT100_BLINK   ""

# define RAVEN_VT100_FG_BLACK   ""
# define RAVEN_VT100_FG_RED     ""
# define RAVEN_VT100_FG_GREEN   ""
# define RAVEN_VT100_FG_YELLOW  ""
# define RAVEN_VT100_FG_BLUE    ""
# define RAVEN_VT100_FG_MAGENTA ""
# define RAVEN_VT100_FG_CYAN    ""
# define RAVEN_VT100_FG_WHITE   ""
# define RAVEN_VT100_FG_DEFAULT ""

# define RAVEN_VT100_BG_BLACK   ""
# define RAVEN_VT100_BG_RED     ""
# define RAVEN_VT100_BG_GREEN   ""
# define RAVEN_VT100_BG_YELLOW  ""
# define RAVEN_VT100_BG_BLUE    ""
# define RAVEN_VT100_BG_MAGENTA ""
# define RAVEN_VT100_BG_CYAN    ""
# define RAVEN_VT100_BG_WHITE   ""
# define RAVEN_VT100_BG_DEFAULT ""

#endif

#endif
