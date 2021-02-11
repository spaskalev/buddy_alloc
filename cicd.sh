#!/bin/bash

ls *.c *.h Makefile | entr -n bash -c 'time make --output-sync --jobs 8'
