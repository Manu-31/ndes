export ROOT_DIR=$(shell pwd)
export SRC_DIR	= src
export INCL_DIR = include
export EXPL_DIR = examples
export TEST_DIR = test
export DOC_DIR = DOCS

export CC=gcc

# Debugage
export CFLAGS=-Wall -g -DDEBUG_NDES
export LDFLAGS=-g  -L../$(SRC_DIR) -lndes -lm

# Performances
#export CFLAGS=-Wall -g -DNDEBUG -O3
#export LDFLAGS=-g -O3 -L../$(SRC_DIR) -lndes -lm

# Génération d'une librairie avec les log intégrés
#export CFLAGS +=  -DNDES_USES_LOG

# En cours de réécriture des événements
#export CFLAGS += -DEVENTS_ARE_NDES_OBJECTS

default : src 

all : src tests examples doc 

.PHONY: clean src examples test

src : 
	@(cd $(SRC_DIR) && $(MAKE))

examples : 
	@(cd $(EXPL_DIR) && $(MAKE))

doc : 
	@(cd $(DOC_DIR) && $(MAKE))

install : src
	@(mkdir lib || true)
	cp src/libndes.a lib

tests-bin : 
	@(cd $(TEST_DIR) && $(MAKE))

tests : tests-bin
	@(cd $(TEST_DIR) && $(MAKE) tests)

clean :
	@(cd $(SRC_DIR) && $(MAKE) $@)
	@(cd $(EXPL_DIR) && $(MAKE) $@)
	@(cd $(TEST_DIR) && $(MAKE) $@)
	@(cd $(DOC_DIR) && $(MAKE) $@)

.c.o :
	$(CC) -I. $< -c

