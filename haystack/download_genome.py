
from haystack_common import initialize_genome
import argparse
import sys
HAYSTACK_VERSION = "0.5.2"
def get_args_download_genome():
    # mandatory
    parser = argparse.ArgumentParser(description='download_genome parameters')
    parser.add_argument('name', type=str,
                        help='genome name. Example: haystack_download_genome hg19.')

    return parser



def main(input_args=None):
    print '\n[H A Y S T A C K   G E N O M E   D O W L O A D E R]\n'
    print 'Version %s\n' % HAYSTACK_VERSION
    parser = get_args_download_genome()
    args = parser.parse_args(input_args)
initialize_genome(args.name)

if __name__ == '__main__':
    main()
    sys.exit(0)