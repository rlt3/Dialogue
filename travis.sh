curl -o luarocks.tar.gz http://keplerproject.github.io/luarocks/releases/luarocks-2.2.2.tar.gz
mkdir luarocks && tar xf luarocks.tar.gz -C luarocks --strip-components 1
cd luarocks/
./configure --lua-version=5.2 --versioned-rocks-dir
make build
sudo make install
