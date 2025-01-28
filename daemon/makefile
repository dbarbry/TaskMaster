NAME = daemon
# *********************************VARIABLES**************************************** #

SRCS_DIR = ./

SRCS =\
		main.cpp \
		parsing.cpp


SRCS := $(SRCS:%=$(SRCS_DIR)/%)

OBJS_DIR = ./.objs

OBJS = $(SRCS:$(SRCS_DIR)/%.cpp=$(OBJS_DIR)/%.o)

INCS = ./incs

DEPS = $(OBJS:.o=.d)

# **********************************COMMANDS**************************************** #

CXX = c++

CXXFLAGS = -Werror -Wextra -Wall

CPPFLAGS = -I$(INCS) -MMD -MP

CFSIGSEV = -Werror -Wextra -Wall -std=c++98 -fsanitize=thread

#VALGRIND = valgrind -q --leak-check=full --show-reachable=yes

RM = rm -rf

DIR_DUP = mkdir -p $(@D)

MAKEFLAGS += --no-print-directory

# **********************************RULES**************************************** #

.PHONY: all
all:	$(NAME)

-include $(DEPS)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OBJS) -o $(NAME)

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@$(DIR_DUP)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: fsanitize
fsanitize : fclean $(OBJS)
	$(CXX) $(CFSIGSEV) $(CPPFLAGS) $(OBJS) -o $(NAME)

.PHONY: clean
clean :
	$(RM) $(OBJS_DIR)

.PHONY: fclean
fclean : clean
	$(RM) $(NAME)

.PHONY: re
re : fclean all

.PHONY: print%
print% :
	@echo $(patsubst print%,%,$@)=
	@echo $($(patsubst print%,%,$@))

.PHONY: FORCE
FORCE:

# Colors
BLACK=\033[30m
RED=\033[31m
GREEN=\033[32m
YELLOW=\033[33m
BLUE=\033[34m
PURPLE=\033[35m
CYAN=\033[36m
WHITE=\033[37m

# Text
ERASE=\033[2K\r
RESET=\033[0m
BOLD=\033[1m
FAINT=\033[2m
ITALIC=\033[3m
UNDERLINE=\033[4m