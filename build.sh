mkdir -p build
mkdir -p build/bin
mkdir -p build/lib
mkdir -p build/include

# Build library.
echo "- Building library"
make # VERBOSE=1
make install

# Build tests.
echo "- Building tests"
cd tests
make # VERBOSE=1
make install
cd ..

# Build examples.
echo "- Building examples"
cd examples
make # VERBOSE=1
make install
cd ..
