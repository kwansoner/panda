CC = g++
CFLAGS = -Wall -pthread  -g

LIBDIR = Lib
BINDIR = Bin
OBJDIR = $(BINDIR)/obj
TARGET = $(BINDIR)/panda

INCLUDE += 	Include/Cond \
			Include/Event \
			Include/Http \
			Include/Log	\
			Include/Mutex \
			Include/Queue \
			Include/Socket \
			Include/ThreadPool \
			Include/Utils

SRCDIR += Src
SRCDIR += Src/Cond
SRCDIR += Src/Event
SRCDIR += Src/Http
SRCDIR += Src/Mutex
SRCDIR += Src/Socket
SRCDIR += Src/ThreadPool

include allrules.mk
