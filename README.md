
# Tor with forwarding delays

ABSTRACT The aim of this project was to come up with a mechanism for introducing micro-delays into *Tor* for the purpose of traffic pattern obfuscation. This mechanism in turn is intended as a building block for future work into the question if stronger anonymity guarantees in the Tor network can be achieved by means of a strategy combining cover traffic with micro-delays.

Note that all tools were developed on/for an AWS EC instance of type z1d.large running Debian 11 Bullseye. For running them on different architectures resp. with another OS may require some adaptations.

## Setting up the environment:

### Shadow:

Install shadow's dependencies:

```
sudo apt-get install -y \
    cmake \
    findutils \
    libclang-dev \
    libc-dbg \
    libglib2.0-0 \
    libglib2.0-dev \
    make \
    python3 \
    python3-pip \
    xz-utils \
    util-linux \
    gcc \
    g++

# rustup: https://rustup.rs
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
# ==> Requires specifying option '1' ...
# .... also need to add 'cargo' to PATH:
source "$HOME/.cargo/env"

# optional python modules
sudo apt-get install -y \
    python3-numpy \
    python3-lxml \
    python3-matplotlib \
    python3-networkx \
    python3-scipy \
    python3-yaml

# optional tools
sudo apt-get install -y \
    dstat \
    git \
    htop \
    tmux   
```

Clone and build shadow:

```
git clone https://github.com/shadow/shadow.git
cd shadow
./setup build --clean --test
./setup test
# Optionally install (to ~/.local/bin by default). Can otherwise run the binary
# directly at build/src/main/shadow.
./setup install

echo 'export PATH="${PATH}:/home/${USER}/.local/bin"' >> ~/.bashrc && source ~/.bashrc
```

### TGen:

Install TGen's dependencies:

```
sudo apt install cmake libglib2.0-0 libglib2.0-dev libigraph1 libigraph-dev -y
```

Clone and build TGen:

```
git clone https://github.com/shadow/tgen.git

cd tgen

mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make

make install
```

### Oniontrace:

Install Oniontrace's dependencies:

```
sudo apt-get install cmake libglib2.0-0 libglib2.0-dev
```

Clone and build Oniontrace:

```
git clone https://github.com/shadow/oniontrace.git

cd oniontrace

mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make

make install
```

### Tornettools:

Clone:

```
git clone https://github.com/shadow/tornettools.git
```

Setup Python3 venv:

```
cd tornettools
python3 -m venv toolsenv
source toolsenv/bin/activate
pip install -r requirements.txt
pip install --ignore-installed .
```

Download and extract the data required by tornettools:

```
wget https://collector.torproject.org/archive/relay-descriptors/consensuses/consensuses-2020-11.tar.xz
wget https://collector.torproject.org/archive/relay-descriptors/server-descriptors/server-descriptors-2020-11.tar.xz
wget https://metrics.torproject.org/userstats-relay-country.csv
wget https://collector.torproject.org/archive/onionperf/onionperf-2020-11.tar.xz
wget -O bandwidth-2020-11.csv "https://metrics.torproject.org/bandwidth.csv?start=2020-11-01&end=2020-11-30"

tar xaf consensuses-2020-11.tar.xz
tar xaf server-descriptors-2020-11.tar.xz
tar xaf onionperf-2020-11.tar.xz

git clone https://github.com/tmodel-ccs2018/tmodel-ccs2018.github.io.git
```

Install additional executables used by tornettools:

```
sudo apt-get install faketime dstat procps xz-utils
```

Move TGEn's tgentools utility as well as Oniontrace's oniontracetools utility into the virtual environment's lib:

```
cp -r $HOME/oniontrace/tools/oniontracetools $HOME/tornettools/toolsenv/lib/python3.9/site-packages
cp -r $HOME/tgen/tools/tgentools $HOME/tornettools/toolsenv/lib/python3.9/site-packages
```

Execute tornettools' stage operation as preparation for network generation:

```
source toolsenv/bin/activate

tornettools stage \
    consensuses-2020-11 \
    server-descriptors-2020-11 \
    userstats-relay-country.csv \
    tmodel-ccs2018.github.io \
    --onionperf_data_path onionperf-2020-11 \
    --bandwidth_data_path bandwidth-2020-11.csv \
    --geoip_path tor/src/config/geoip
```

### Tor

Install packages required by Tor:

```
sudo apt install autotools-dev automake clang cmake libevent-dev libssl-dev zlib1g zlib1g-dev -y
```

Build the three variants of the Tor codebase and move the generated binaries to dedicated directories in $HOME/.local/bin:

### `delay_neg_32`: Modified Tor codebase including the entire delay mechanism described in report.

In case you're wondering how the moniker came into existence, it stands for delay negotiate 32 values (arrays containing 32 sampled delays each are supplied by client).

```
cd delay_neg_32

./autogen.sh

./configure --disable-asciidoc --prefix=/home/admin/.local

make

sudo make install

mkdir $HOME/.local/bin/delay_neg_32
mv -t $HOME/.local/bin/delay_neg_32 $HOME/.local/bin/tor $HOME/.local/bin/torify $HOME/.local/bin/tor-resolve $HOME/.local/bin/tor-gencert $HOME/.local/bin/tor-print-ed-signing-cert

cd ..
```
### `delay_neg_32_zero`: Modified Tor codebase including the entire overhead incurred by sending and applying delays, but without actually applying delays.

In case you're wondering how the moniker came into existence, it stands for delay negotiate 32 all-zero values (all-zero arrays of size 32 values each are supplied by client).

```
cd delay_neg_32_zero

./autogen.sh

./configure --disable-asciidoc --prefix=/home/admin/.local

make

sudo make install

mkdir $HOME/.local/bin/delay_neg_32_zero
mv -t $HOME/.local/bin/delay_neg_32_zero $HOME/.local/bin/tor $HOME/.local/bin/torify $HOME/.local/bin/tor-resolve $HOME/.local/bin/tor-gencert $HOME/.local/bin/tor-print-ed-signing-cert

cd ..
```
### `tor_unchanged`: Unmodified Tor codebase.

```
cd tor_unchanged

./autogen.sh

./configure --disable-asciidoc --prefix=/home/admin/.local

make

sudo make install

mkdir $HOME/.local/bin/tor_unchanged
mv -t $HOME/.local/bin/tor_unchanged $HOME/.local/bin/tor $HOME/.local/bin/torify $HOME/.local/bin/tor-resolve $HOME/.local/bin/tor-gencert $HOME/.local/bin/tor-print-ed-signing-cert

cd ..
```

## Running the experiment:

Start a sudo shell:

```
sudo -s
```

Adjust root's PATH:

```
export PATH=/home/admin/.local/bin:/home/admin/.cargo/bin:/home/admin/tgen/tools/tgentools:/home/admin/oniontrace/tools/oniontracetools:/root/.local/bin:/home/admin/tornettools/tor/src/core/or:/home/admin/tornettools/tor/src/tools:$PATH
```

Activate the virtual environment generated inside the tornettools repository:

```
source tornettools/toolsenv/bin/activate
```

If not existent, make directory for root's binaries:

```
mkdir /root/.local/bin
```

Adjust paths in `run_experiment.py` as needed; as is, it is assumed the script is run inside $HOME.

Run: 

```
python run_experiment.py
```

## Analyzing the results:

Assumption: simulation results are in `$(cwd)/analysis/sim_res/{delay_neg_32, delay_neg_32_zero, tor_unchanged}/{599, 873, 953, 1012, 1294, 6938}`

Deactvate tornettools' virtual environment, enter analysis directory, and set up a new virtual environment.

```
deactivate

cd analysis

python3 -m venv analysis_venv

source analysis_venv/bin/activate

python -m pip install -r requirements.txt
```

Run the `download_time.ipynb` notebook to generate results. Retrieve plots from `analysis/plots`.