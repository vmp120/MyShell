echo REDIRECTIONTEST

# create new p1.txt
touch p1.txt

# redirect "hello" into p1.txt
echo hello > p1.txt

# "hello" should print to terminal
cat < p1.txt

# create new p2.txt
touch p2.txt

# redirect "input" into p2.txt
echo input > p2.txt

# create new p3.txt
touch p3.txt

# copy "input" into p3.txt
cat < p2.txt > p3.txt

# print "input" to terminal
cat p3.txt

# remove all test files
rm p1.txt
rm p2.txt
rm p3.txt

echo COMPLETE