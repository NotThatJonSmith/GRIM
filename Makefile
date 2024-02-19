CXXFLAGS = -O3 -std=c++20 -Wall -Wextra -Wno-unused-parameter -Werror -pedantic

grim: grim.cpp
	${CXX} ${CXXFLAGS} -Iinclude -Iexternal/cxxopts/include -Iexternal/HighELF/include $< -o $@

all: grim

