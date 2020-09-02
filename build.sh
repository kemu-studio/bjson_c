mkdir -p build
mkdir -p build/bin
mkdir -p build/lib
mkdir -p build/include

# Build library.
cd src
make # VERBOSE=1
make install
cd ..

# Build tests.
cd tests
make # VERBOSE=1
make install
cd ..

# Build examples.
cd examples
make # VERBOSE=1
make install
cd ..
