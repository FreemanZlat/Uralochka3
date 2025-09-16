for f in data_66000/*.npy;
do
  if [ -f "$f" ]
  then
    echo "Processing $f file..."
    ./Uralochka3-avx2 reeval 12 $f new_$f
    echo ""
  else
    echo "Warning: Some problem with \"$f\""
  fi
done
