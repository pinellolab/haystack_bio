############################################################
# Dockerfile to build haystack_bio
# Based on Ubuntu 16.04
############################################################

# Set the base image to Ubuntu
FROM ubuntu:16.04 

# Set the working directory to /haystack_bio
WORKDIR /haystack_bio

# Copy the current directory contents into the container at /haystack_bio
COPY . /haystack_bio 

ENV PATH /haystack_bio/binaries:$PATH
RUN ln -s /haystack_bio/binaries/sambamba_v0.6.6 /haystack_bio/binaries/sambamba

RUN apt-get update && \
    apt-get install -y bedtools default-jre python-setuptools python-pip python-dev python-numpy python-scipy python-matplotlib python-pandas python-imaging unzip libxml2-dev libxslt1-dev zlib1g-dev zlib1g libexpat1-dev libhtml-template-perl libxml-simple-perl libsoap-lite-perl imagemagick python-setuptools && \
    pip install bx-python weblogo pybedtools tqdm Jinja2 && \
    cpan Log::Log4perl HTML::PullParser HTML::Parse CGI::Application common::sense Types::Serialiser  XML::Compile::SOAP11 XML::Compile::WSDL11 XML::Compile::Transport::SOAPHTTP Math::CDF JSON && \
    apt-get remove -y python-pip && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /haystack_bio/binaries/meme_4.12.0 
RUN ./configure --prefix=/haystack_bio/binaries/meme   && \
    make clean && \
    make AM_CFLAGS='-DNAN="(0.0/0.0)"' && \
    make install

ENV PATH /haystack_bio/binaries/meme/bin:$PATH
RUN  rm -rf /haystack_bio/binaries/meme_4.12.0

WORKDIR /haystack_bio
RUN python setup.py install
RUN haystack_download_genome hg19 --yes



