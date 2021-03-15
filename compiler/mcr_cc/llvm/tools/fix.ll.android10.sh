#!/bin/bash

TMP_FIX_FILE=tmp.fix
MODULE_FULL=art_module_full.ll 
MODULE=art_module.ll 
OUTPUT_FILE=test.ll

cp $MODULE_FULL ../$MODULE

echo "Fixing generated bitcode for LLVM v10"
cat ../$MODULE \
    | sed 's|\(^.*\)\( local_unnamed_addr \#[0-9][0-9]*\)\(.*$\)|\1\3|g' \
    | sed 's|\(^.*\)\( dso_local \)\(.*$\)|\1 \3|g' \
    | sed 's|\(^.*\)\( noinline\)\(.*$\)|\1\3|g' \
    | sed 's|\(^.*\)\( optnone\)\(.*$\)|\1\3|g' \
    | sed 's|\(^source_filename\)\(.*$\)|;\1\2|g' \
    | sed 's|\(^target datalayout\)\(.*$\)|;\1\2|g' \
    | sed 's|\(^target triple\)\(.*$\)|;\1\2|g' \
    | sed 's|\(^\!\)\(.*$\)|;\1\2|g' \
    >> ${TMP_FIX_FILE}

cp $TMP_FIX_FILE ../$MODULE
rm $TMP_FIX_FILE

echo "Fixed LLVM bitcode for android10/LLVM9:"
