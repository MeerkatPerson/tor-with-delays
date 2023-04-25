
import subprocess

# NOTE: it is assumed that ...
# (1) all of the three Tor variants (delay_neg_32, delay_neg_32_zero, tor_unchanged), tornettools, tgen, oniontrace, this script itself, as well as reverted_traffic.tgenrc.graphml are located in $HOME. If they aren't, adjust paths accordingly.
# (2) this script is run from a root shell and root's PATH has been adapted as instructed in README.

dirs = ["delay_neg_32", "delay_neg_32_zero", "tor_unchanged"]

seeds = [953, 1294, 599, 1012, 6938, 873]

subprocess.run(
    [f'cp $HOME/.local/bin/shadow /root/.local/bin/shadow'], shell=True)

subprocess.run(
    [f'cp $HOME/.local/bin/tgen /root/.local/bin/tgen'], shell=True)

subprocess.run(
    [f'cp $HOME/.local/bin/oniontrace /root/.local/bin/oniontrace'], shell=True)

# Generate one tornet for every seed
for ind in seeds:

    subprocess.run([f"tornettools --seed={ind} generate \
    relayinfo_staging_2020-11-01--2020-11-30.json \
    userinfo_staging_2020-11-01--2020-11-30.json \
    networkinfo_staging.gml \
    tmodel-ccs2018.github.io \
    --network_scale 0.0001 \
    --prefix tornet-0.0001-{ind}"], shell=True, cwd="$HOME/tornettools")

    # !!!!! CRUCIAL STEP: adapt the tgen conf files to revert traffic direction !!!!
    subprocess.run(
        [f'cp $HOME/reverted_traffic.tgenrc.graphml $HOME/tornettools/tornet-0.0001-{ind}/conf/tgen-perf-exit.tgenrc.graphml'], shell=True)

# now simulate each Tor version on each of the generated networks
for dir in dirs:

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # (I) firstly, ensure binaries in .local/bin are the ones corresponding to #     dir

    for bin in ["tor", "torify", "tor-gencert", "tor-resolve", "tor-print-ed-signing-cert"]:

        # remove the version of bin currently lying in top level dir of admin
        subprocess.run([f'rm $HOME/.local/bin/{bin}'], shell=True)

        # move the version of bin corresponding to the implementation in dir
        # into $HOME/.local/bin
        subprocess.run(
            [f'cp $HOME/.local/bin/{dir}/{bin} $HOME/.local/bin/{bin}'], shell=True)

        # remove the version of bin currently lying in top level dir of root
        subprocess.run([f'rm /root/.local/bin/{bin}'], shell=True)

        # move the version of bin corresponding to the implementation in dir
        # into /root/.local/bin
        subprocess.run(
            [f'cp $HOME/.local/bin/{dir}/{bin} /root/.local/bin/{bin}'], shell=True)

     # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     # (II) make a directory which will contain the tornet.plot.data for each
     #      of the generated tornets for this implementation

    subprocess.run(
        [f'mkdir $HOME/tornettools/{dir}-plot-data'], shell=True)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # (III) Ensure the correct version of Tor is located in tornettools/tor
    subprocess.run(['yes | rm -r $HOME/tornettools/tor'], shell=True)

    subprocess.run(
        [f'cp -r $HOME/{dir} $HOME/tornettools/tor'], shell=True)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # (III) now, for each of the generated tornets:
    for ind in seeds:

        subprocess.run(
            [f'mkdir $HOME/tornettools/{dir}-plot-data/results-{ind}'], shell=True)

        # (a) simulate
        subprocess.run(
            [f'tornettools --seed={ind} simulate tornet-0.0001-{ind}'], shell=True, cwd='$HOME/tornettools')

        # (b) parse
        subprocess.run(
            [f'tornettools --seed={ind} parse tornet-0.0001-{ind}'], shell=True, cwd='$HOME/tornettools')

        # (c) move resulting tornet.plot.data directory into the directory which
        #     will contain all tornet.plot.data dirs for the different generated
        #     tornets for this implementation
        subprocess.run(
            [f'mv $HOME/tornettools/tornet-0.0001-{ind}/tornet.plot.data $HOME/tornettools/{dir}-plot-data/results-{ind}'], shell=True)

        # ********************************************************************
        # (d) move all the other results into respective dir as well
        subprocess.run(
            [f'mv $HOME/tornettools/tornet-0.0001-{ind}/free_rusage.json.xz $HOME/tornettools/{dir}-plot-data/results-{ind}'], shell=True)

        subprocess.run(
            [f'mv $HOME/tornettools/tornet-0.0001-{ind}/shadow_rusage.json.xz $HOME/tornettools/{dir}-plot-data/results-{ind}'], shell=True)

        subprocess.run(
            [f'mv $HOME/tornettools/tornet-0.0001-{ind}/tgen.analysis.json.xz $HOME/tornettools/{dir}-plot-data/results-{ind}'], shell=True)

        subprocess.run(
            [f'mv $HOME/tornettools/tornet-0.0001-{ind}/oniontrace.analysis.json.xz $HOME/tornettools/{dir}-plot-data/results-{ind}'], shell=True)

        # ********************************************************************
        # (e) parse server tgen data

        subprocess.run([f"tgentools parse -m 1 -e 'server[0-9]+exit\.tgen\.[0-9]+\.stdout' --complete shadow.data/hosts"],
                       shell=True, cwd=f'$HOME/tornettools/tornet-0.0001-{ind}')

        subprocess.run(
            [f'mv $HOME/tornettools/tornet-0.0001-{ind}/tgen.analysis.json.xz $HOME/tornettools/{dir}-plot-data/results-{ind}/tgen_servers.analysis.json.xz'], shell=True)

        # ********************************************************************

        # clean up our tornet
        subprocess.run(
            [f'yes | rm -r $HOME/tornettools/tornet-0.0001-{ind}/shadow.data'], shell=True)
