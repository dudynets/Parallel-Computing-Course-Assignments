# Create dist folder if not exists
mkdir -p dist

# Compile the code
g++-14 app.cpp -o dist/app

# Run the code with arguments
./dist/app "$@"
