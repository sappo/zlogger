echo "Generate Space-Time Diagram${STD}";
#  Generate Time Space Diagram
echo "digraph G {" > dia_space_time.dot
for sdot in *.sdot;
do
    cat $sdot >> dia_space_time.dot
    rm $sdot
done
echo "}" >> dia_space_time.dot
dot -Tsvg dia_space_time.dot > dia_space_time.svg
