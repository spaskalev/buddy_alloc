#!/bin/bash

ls *.c *.h Makefile | entr -n make
