#!/usr/bin/env bash 
# Install haystack_bio and its dependencies on Ubuntu 16.04
if test "haystack_hotspots -h" ; then
    echo "haystack_bio already installed."
else
    echo "Installing haystack."
    if [[ -d $HOME/haystack_bio ]] ; then 
    rm -rf $HOME/haystack_bio ; fi
	sudo apt-get update 
	sudo apt-get install -y \
	python-dev \
	gfortran \
	libopenblas-dev \
	liblapack-dev \
	subversion \
	bedtools \
	curl \
	unzip \
	zlib1g-dev \
	default-jre \
	libexpat1-dev \
	libhtml-template-perl \
	libxml-simple-perl \
	libsoap-lite-perl \
	libxml2-dev \
	libxslt1-dev \
	python-pip \
    python-numpy \
    python-scipy\
    python-matplotlib \
    python-pandas \
	python-setuptools 
	sudo pip install \
	bx-python \
	Jinja2 \
	tqdm \
	weblogo
	sudo cpan \
    common::sense \
	CGI::Application \
	Log::Log4perl \
	HTML::PullParser \
	HTML::Parse \
	Math::CDF JSON \
	Types::Serialiser  \
	XML::Compile::SOAP11 \
	XML::Compile::WSDL11 \
	XML::Compile::Transport::SOAPHTTP
    curl -fL  https://github.com/pinellolab/haystack_bio/archive/0.5.2.tar.gz \
		-o $HOME/haystack_bio.tar.gz \
    tar -xzf $HOME/haystack_bio.tar.gz -C $HOME 
    mv  $HOME/haystack_bio-0.5.0 $HOME/haystack_bio 
	mkdir -p $HOME/haystack_bio/binaries
	curl -fL http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/bedGraphToBigWig \
		-o $HOME/haystack_bio/binaries/bedGraphToBigWig  \
    chmod +x $HOME/haystack_bio/binaries/bedGraphToBigWig 
	curl -fL http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/bigWigAverageOverBed \
		-o $HOME/haystack_bio/binaries/bigWigAverageOverBed  
    chmod +x $HOME/haystack_bio/binaries/bigWigAverageOverBed 
	curl -fL  https://github.com/lomereiter/sambamba/releases/download/v0.6.6/sambamba_v0.6.6_linux.tar.bz2 \
		-o $HOME/haystack_bio/binaries/sambamba_v0.6.6_linux.tar.bz2 
	tar -xvjf $HOME/haystack_bio/binaries/sambamba_v0.6.6_linux.tar.bz2 -C $HOME/haystack_bio/binaries 
	rm -f $HOME/haystack_bio/binaries/sambamba_v0.6.6_linux.tar.bz2 
    ln -s $HOME/haystack_bio/binaries/sambamba_v0.6.6 $HOME/haystack_bio/binaries/sambamba 
	curl -fL http://alternate.meme-suite.org/meme-software/4.12.0/meme_4.12.0.tar.gz \
		-o $HOME/haystack_bio/binaries/meme_4.12.0.tar.gz  \
	tar -xzf $HOME/haystack_bio/binaries/meme_4.12.0.tar.gz -C $HOME/haystack_bio/binaries 
	rm -f $HOME//haystack_bio/binaries/meme_4.12.0.tar.gz

	cd $HOME/haystack_bio/binaries/meme_4.12.0 
	./configure --prefix=/haystack_bio/binaries/meme
	make clean
	make AM_CFLAGS='-DNAN="(0.0/0.0)"'
	make install 
	rm -rf /haystack_bio/binaries/meme_4.12.0
    cd $HOME/haystack_bio
    export PATH=$HOME/haystack_bio/binaries:$HOME/haystack_bio/binaries/meme/bin:$PATH
	python setup.py install
fi

#wget -c https://raw.githubusercontent.com/pinellolab/haystack_bio/master/manual_build.sh -O $HOME/manual_build.sh
#chmod +x $HOME/manual_build.sh
#$HOME/manual_build.sh -b
#export PATH=$HOME/haystack_bio/binaries:$HOME/haystack_bio/binaries/meme/bin:$PATH



