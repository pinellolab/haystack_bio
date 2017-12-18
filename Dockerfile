############################################################
# Dockerfile to build haystack_bio
# Based on Ubuntu 16.04
############################################################

# Set the base image to Ubuntu
FROM ubuntu:16.04

RUN apt-get update \
	&& apt-get install -y \
	python-dev \
	gfortran \
	libopenblas-dev \
	liblapack-dev \
	zlib1g-dev \
	libexpat1-dev \
	libhtml-template-perl \
	libxml-simple-perl \
	libsoap-lite-perl \
	libxml2-dev \
	libxslt1-dev \
	default-jre \
	subversion \
	curl \
	python-pip \
	unzip \
	bedtools \
	ghostscript \
	&& rm -rf /var/lib/apt/lists/* \
    && python -m pip install --user \
    setuptools==37.0.0 \
    numpy==1.13.3 \
  	scipy==1.0.0 \
  	matplotlib==2.1.0 \
  	pandas==0.21.0 \
    && pip install \
	bx-python==0.7.3 \
	Jinja2==2.9.6 \
	tqdm==4.19.4 \
	weblogo==3.5.0 \
    && cpan \
	inc::latest \
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

RUN mkdir -p /haystack_bio/binaries \
	&& curl -fL http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/bedGraphToBigWig \
		-o /haystack_bio/binaries/bedGraphToBigWig  \
    && chmod +x /haystack_bio/binaries/bedGraphToBigWig \
	&& curl -fL http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/bigWigAverageOverBed \
		-o /haystack_bio/binaries/bigWigAverageOverBed  \
    && chmod +x /haystack_bio/binaries/bigWigAverageOverBed \
	&& curl -fL  https://github.com/lomereiter/sambamba/releases/download/v0.6.6/sambamba_v0.6.6_linux.tar.bz2 \
		-o /haystack_bio/binaries/sambamba_v0.6.6_linux.tar.bz2 \
	&& tar -xvjf /haystack_bio/binaries/sambamba_v0.6.6_linux.tar.bz2 -C /haystack_bio/binaries \
	&& rm -f /haystack_bio/binaries/sambamba_v0.6.6_linux.tar.bz2 \
    && ln -s /haystack_bio/binaries/sambamba_v0.6.6 /haystack_bio/binaries/sambamba \
	&& curl -fL http://meme-suite.org/meme-software/4.11.2/meme_4.11.2_2.tar.gz \
		-o /haystack_bio/binaries/meme_4.11.2_2.tar.gz  \
	&& tar -xzf /haystack_bio/binaries/meme_4.11.2_2.tar.gz -C /haystack_bio/binaries \
	&& rm -f /haystack_bio/binaries/meme_4.11.2_2.tar.gz 

WORKDIR /haystack_bio/binaries/meme_4.11.2

RUN ./configure --prefix=/haystack_bio/binaries/meme \
	&& make clean \
	&& make AM_CFLAGS='-DNAN="(0.0/0.0)"' \
	&& make install \
	&& rm -rf /haystack_bio/binaries/meme_4.11.2

RUN apt-get remove -y python-pip curl && apt-get clean

# Set the working directory to /haystack_bio
WORKDIR /haystack_bio_setup

# Copy the current directory contents into the container at /haystack_bio
COPY . /haystack_bio_setup

ENV PATH /haystack_bio/binaries:/haystack_bio/binaries/meme/bin:$PATH

RUN python setup.py install

RUN  ln -s /usr/local/lib/python2.7/dist-packages/haystack_bio-0.5.3-py2.7.egg/haystack/haystack_data/genomes/ /haystack_genomes

RUN rm -Rf /haystack_bio_setup
