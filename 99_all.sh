# Make shell script exit on failures and add some verbosity
set -exv

source ./01_prepare.sh
source ./02_build.sh
source ./03_test.sh