#!/bin/bash
rm -rf ../../exports/woodlands
mkdir -p ../../exports/woodlands
../../desktop/export data.txt > ../../exports/woodlands/data.txt
cp sprites/*png ../../exports/woodlands/
cp ../../web/* ../../exports/woodlands
