#!/bin/bash

ls *.c *.h Makefile | entr -n make --output-sync --jobs 8
