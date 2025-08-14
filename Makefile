BBLU		=	\033[1;34m
BGREEN		=	\033[1;32m
BRED		=	\033[1;31m
BLU			=	\033[0;34m
GRN			=	\033[0;32m
RED			=	\033[0;31m
RST			=	\033[0m

SRC_CLIENT		=	$(shell find ./$(NAME_CLIENT) -type f -name "*.cpp" | cut -c 10-)
HDR_CLIENT		= 	$(shell find ./$(NAME_CLIENT) -type f -name "*.hpp" | cut -c 3-)

SRC_SERVER		=	$(shell find ./$(NAME_SERVER) -type f -name "*.cpp" | cut -c 10-)
HDR_SERVER		= 	$(shell find ./$(NAME_SERVER) -type f -name "*.hpp" | cut -c 3-)

OBJ_CLIENT		=	$(addprefix ./$(OBJ_CLIENT_REP)/, $(SRC_CLIENT:.cpp=.o))
OBJ_SERVER		=	$(addprefix ./$(OBJ_SERVER_REP)/, $(SRC_SERVER:.cpp=.o))

CXX				=	g++
FLAGS			=	-std=c++20 #-Wall -Werror -Wextra -std=c++20
RDLINE_FLAGS	=	-lreadline 
HDR_FLAGS_D		=	-I daemon/
HDR_FLAGS_C		=	-I client/

TSAN_FLAGS       = -fsanitize=thread -g -fno-omit-frame-pointer
ASAN_FLAGS       = -fsanitize=address -g -fno-omit-frame-pointer
UBSAN_FLAGS      = -fsanitize=undefined -g -fno-omit-frame-pointer

NAME			=	taskmaster
NAME_CLIENT		=	client
NAME_SERVER		=	daemon

OBJ_CLIENT_REP	=	obj_$(NAME_CLIENT)
OBJ_SERVER_REP	=	obj_$(NAME_SERVER)
NAME_LOG		=	log

all: print_header $(NAME)
.PHONY: all

print_header:
	@echo "$(BBLU)==========================================================="
	@echo "$(BBLU)"
	@echo "$(BBLU)\033[5G  _______        _    __  __           _            "
	@echo "$(BBLU)\033[5G |__   __|      | |  |  \/  |         | |           "
	@echo "$(BBLU)\033[5G    | | __ _ ___| | _| \  / | __ _ ___| |_ ___ _ __ "
	@echo "$(BBLU)\033[5G    | |/ _\` / __| |/ / |\/| |/ _\` / __| __/ _ \ '__|"
	@echo "$(BBLU)\033[5G    | | (_| \__ \   <| |  | | (_| \__ \ ||  __/ |   "
	@echo "$(BBLU)\033[5G    |_|\__,_|___/_|\_\_|  |_|\__,_|___/\__\___|_|   "                                        
	@echo "$(BBLU)"
	@echo "$(BBLU)==================> by rgeral & dbarbry <=================="
	@echo "$(RST)"
.PHONY: print_header

prerequisities:
	sudo apt-get install libreadline-dev
.PHONY: prerequisities

ifeq (server,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif

./obj_client/%.o: ./$(NAME_CLIENT)/%.cpp $(HDR_CLIENT)
	mkdir -p $(OBJ_CLIENT_REP) $(OBJ_CLIENT_REP)/utils
	$(CXX) $(FLAGS) $(HDR_FLAGS_C) -c $< -o $@
	echo "$(BBLU)[$(NAME) OBJ] :$(RST) $@ $(BGREEN)\033[56G[✔]$(RST)"

./obj_daemon/%.o: ./$(NAME_SERVER)/%.cpp $(HDR_SERVER)
	mkdir -p $(OBJ_SERVER_REP) $(OBJ_SERVER_REP)/cmds
	mkdir -p $(OBJ_SERVER_REP) $(OBJ_SERVER_REP)/cmds/cmd
	mkdir -p $(OBJ_SERVER_REP) $(OBJ_SERVER_REP)/launch
	$(CXX) $(FLAGS) $(HDR_FLAGS_D) -c $< -o $@
	echo "$(BBLU)[$(NAME) OBJ] :$(RST) $@ $(BGREEN)\033[56G[✔]$(RST)"

$(NAME): $(OBJ_CLIENT) $(OBJ_SERVER)
	$(CXX) $(FLAGS) $(HDR_FLAGS_C) $(OBJ_CLIENT) $(RDLINE_FLAGS) -o $(NAME_CLIENT).out
	echo "$(BGREEN)[$(NAME) END] :$(RST)$(RST) ./$(NAME_CLIENT).out $(BGREEN)\033[56G[✔]$(RST)"
	$(CXX) $(FLAGS) $(HDR_FLAGS_D) $(OBJ_SERVER) -o $(NAME_SERVER).out
	echo "$(BGREEN)[$(NAME) END] :$(RST)$(RST) ./$(NAME_SERVER).out $(BGREEN)\033[56G[✔]$(RST)"
.PHONY: $(NAME)

client:
	@if [ ! -f "./$(NAME_CLIENT).out" ]; then \
    	echo "$(RED)[ERROR] :$(RST) Compile the project first$(RED)\033[56G[✘]$(RST)"; \
    	exit 1; \
    fi
	@echo "$(GRN)[LOG]  :$(RST) Launching $(NAME_CLIENT).out...$(BGREEN)\033[56G[✔]$(RST)"
	@./$(NAME_CLIENT).out || true
.PHONY: client

server:
	@PID=$$(ps -eo pid,comm | grep "[d]aemon.out" | awk '{print $$1}'); \
    if [ "$$PID" ]; then \
        echo "$(RED)[ERROR] :$(RST) A server is already running$(RED)\033[56G[✘]$(RST)"; \
        exit 1; \
    fi; \
    if [ ! -f "./$(NAME_SERVER).out" ]; then \
        echo "$(RED)[ERROR] :$(RST) Compile the project first$(RED)\033[56G[✘]$(RST)"; \
        exit 1; \
    fi
	@echo "$(GRN)[LOG]  :$(RST) Launching $(NAME_SERVER).out...$(BGREEN)\033[56G[✔]$(RST)"
	@./$(NAME_SERVER).out $(RUN_ARGS) || true
.PHONY: server

kill:
	@PID=$$(ps -eo pid,args | grep "[d]aemon.out" | grep -v grep | awk '{print $$1}'); \
	if [ -z "$$PID" ]; then \
		echo "$(RED)[ERROR] :$(RST) No running daemon found$(RED)\033[56G[✘]$(RST)"; \
	else \
		if ! sudo kill $$PID 2>/dev/null; then \
			echo "$(RED)[ERROR] :$(RST) Failed to kill daemon (need root?)$(RED)\033[56G[✘]$(RST)"; \
			exit 1; \
		fi; \
		echo "$(GRN)[LOG]  :$(RST) Stopping daemon (PID: $$PID)...$(BGREEN)\033[56G[✔]$(RST)"; \
	fi
.PHONY: kill

clean:
	$(RM) $(OBJ_CLIENT) $(OBJ_SERVER)
	$(RM) -r $(OBJ_CLIENT_REP) $(OBJ_SERVER_REP)
	echo "$(RED)[CLEAN]  :$(RST) Deleting objects...$(BGREEN)\033[56G[✔]$(RST)"
.PHONY: clean

clean_log:
	$(RM) -r $(NAME_LOG)
	echo "$(RED)[FCLEAN] :$(RST) Deleting logs...$(BGREEN)\033[56G[✔]$(RST)"

fclean: clean
	$(RM) $(NAME_CLIENT).out $(NAME_SERVER).out
	echo "$(RED)[FCLEAN] :$(RST) Deleting executable...$(BGREEN)\033[56G[✔]$(RST)"
.PHONY: fclean

re: fclean
	$(MAKE) all
.PHONY: re

# Modification de la cible tsan pour utiliser la variable make directement
tsan: fclean
	@$(MAKE) all FLAGS="$(FLAGS) $(TSAN_FLAGS)"
	@echo "$(BGREEN)[INFO] :$(RST) Compiled with ThreadSanitizer$(BGREEN)\033[56G[✔]$(RST)"
.PHONY: tsan

# Ajout d'une cible pour compiler sans sanitizer
nosani: fclean
	@$(MAKE) all FLAGS="$(filter-out $(TSAN_FLAGS), $(FLAGS))"
	@echo "$(BGREEN)[INFO] :$(RST) Compiled without sanitizers$(BGREEN)\033[56G[✔]$(RST)"
.PHONY: nosani

# Modification similaire pour les autres sanitizers
asan: fclean
	@$(MAKE) all FLAGS="$(FLAGS) $(ASAN_FLAGS)"
	@echo "$(BGREEN)[INFO] :$(RST) Compiled with AddressSanitizer$(BGREEN)\033[56G[✔]$(RST)"
.PHONY: asan

ubsan: fclean
	@$(MAKE) all FLAGS="$(FLAGS) $(UBSAN_FLAGS)"
	@echo "$(BGREEN)[INFO] :$(RST) Compiled with UndefinedBehaviorSanitizer$(BGREEN)\033[56G[✔]$(RST)"
.PHONY: ubsan

.SILENT: