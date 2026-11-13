#!/bin/bash

echo "# qslackpkg" >> README.md
git init
git add README.md
git commit -m "first commit"
git branch -M main
git remote add origin https://github.com/develslack/qslackpkg.git
git push -u origin main
