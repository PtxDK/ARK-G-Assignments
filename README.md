# ARK-G-Assignments

## How To Use Git with GitHub

###### Install Git Package on Debian
* sudo apt-get install git-core

###### Download repository from github
* git clone https://github.com/PtxDK/ARK-G-Assignments


###### Add changes to repository
* git add .             // Adds everything to next push
* git status            // shows changes you are about to commit
* git commit -m "Some Comment"  // Prepare for push
* git push            // You are sending all chosen changes to your repository


###### Remove files from repository
* git rm filemane         // Removes file from repo on next push
* git status            // Shows changes you are about to commit
* git commit -m "Some Comment"  // Prepare for push
* git push            // you are sending all chosen changes to your repository


###### If greater changes has been made such as folder changes
* git add . --all         // Removes everything from repository and replaces it with your folder
* git status            // Shows changes you are about to commit
* git commit -m "Some Comment"  // Prepare for push
* git push            // You are sending all chosen changes to your repository

##### Branching

###### Create and start working on new branch
* git checkout -b branchname

###### Deletion of Branch
* git branch -d branchname

###### Merge branch into master branch
* git checkout master
* git merge branchname





