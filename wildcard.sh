#wildcard test script, only appears in file names and not paths
echo WILDCARDTEST

#show current directory
pwd

#create test files
touch test1.txt
touch test2.txt

#test for no matching files
#should remain unchanged
ls *.doesntexist

#wildcard at the end of the filename
ls file1*.txt

#wildcard at the beginning of the token
ls *.file1

#special characters like \*test2.txt
ls *test2.txt

#remve all test files
rm test1.txt
rm test2.txt

#done
ech COMPLETE

