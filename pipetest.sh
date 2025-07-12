echo PIPETESTING

# print a sequence of words normally
echo -e apple\napple\nbanana\nbanana\norange

# print a sequence of words in alphabetical order
echo -e apple\napple\nbanana\nbanana\norange | sort

#create a new txt file
touch p1.txt

# redirect sequence of words into the txt file
echo -e apple\napple\nbanana\nbanana\norange > p1.txt

# print words from txt file normally
cat p1.txt

# print words from txt file in alphabetical order
cat p1.txt | sort

# print unique words from txt file
cat p1.txt | uniq

# remove the txt file now that testing is complete
rm p1.txt
echo COMPLETE