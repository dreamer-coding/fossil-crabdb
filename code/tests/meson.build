if get_option('with_test').enabled()
    run_command(['python3', 'tools' / 'generate-runner.py'], check: true)

    c_test_src = ['unit_runner.c']
    cpp_test_src = ['unit_runner.cpp']

    subdir('with_c')
    subdir('with_cpp')

    test('C API test cases',   c_pizza)
    test('C++ API test cases', cpp_pizza)
endif
