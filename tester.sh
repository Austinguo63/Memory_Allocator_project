#! /bin/bash

total_tests=0
passed_tests=0
failed_tests=0

for output_name in sample_output/*; do
    tokens=(${output_name//-/ })
    type=${tokens[1]}
    input_name=${tokens[2]} 

    ((total_tests=total_tests+1))
    
    ./proj2 $type sample_input/$input_name > /dev/null
    if [ $? -eq 0 ]; then
        cat output/result-$type-$input_name > output.tmp
        cat sample_output/result-$type-$input_name > expected.tmp
        diff output.tmp expected.tmp
        if [ $? -eq 0 ]; then
            echo PASSED
            ((passed_tests=passed_tests+1))
        else
            echo OUTPUT_MISMATCH
            echo $type-$input_name
            ((failed_tests=failed_tests+1))
        fi
    else
        echo PROGRAM_CRASHED
        echo $type-$input_name
        ((failed_tests=failed_tests+1))
    fi
done

echo TOTAL_TESTS: $total_tests
echo PASSED_TESTS: $passed_tests
echo FAILED_TESTS: $failed_tests

rm *.tmp