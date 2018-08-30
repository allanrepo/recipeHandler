TOOL_TARGET = recipeHandler


# -- Start of project files --
PROJECT_SOURCES =	recipeHandler.cpp \
			userInterface.cpp \
			argSwitches.cpp \
			evxaTester.cpp \
			recipeSupport.cpp \
			xmlInterface.cpp \


PROJECT_HEADERS   =	cmdLineSwitches.h \
			evxaTester.h \
			argSwitches.h \
			userInterface.h \
			recipeSupport.h \
			xmlInterface.h \


EVXA_INCLUDES = -I/ltx/customer/include/

# Even though this is not a CURI project, there are some headers in there that make life simple.
PROJECT_INCLUDE_PATHS = $(EVXA_INCLUDES)

MBITS=64
PROJECT_LIBRARIES=-Wl,-rpath,/ltx/lib$(MBITS) -L/ltx/lib$(MBITS) -levxa

CFLAGS=-D_USE_NEW_CURI_LOCK_

ifeq ("$(BUILD_OS)", "Linux")
CFLAGS+=-pthread -DLINUX_TARGET
endif


ifeq ("$(CFG)", "Debug")
# LDFLAGS:=-Wl,-rpath,'$$ORIGIN'
endif

