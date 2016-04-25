'''
Created on May 3, 2011

@author: Luca Pinello
'''
import mmap
import math
import os, glob, string


import numpy as np
from numpy import zeros, inf, log2
from numpy.random import  randint,permutation

from scipy.stats import rv_discrete
from scipy.io.matlab import savemat

import subprocess
import tempfile
import time

from bx.intervals.intersection import Intersecter, Interval
from bx.seq.twobit import TwoBitFile


import cPickle

try:
    import pysam
except:
    pass

mask=lambda c: c if c.isupper() else 'N' 

def chunks(l, n):
 return [l[i:i+n] for i in range(0, len(l), n)]

class constant_minus_one_dict (dict):
    def __missing__ (self, key):
        return -1

class constant_n_dict (dict):
    def __missing__ (self, key):
        return 'n'

nt2int=constant_minus_one_dict({'a':0,'c':1,'g':2,'t':3})      
int2nt=constant_n_dict({0:'a',1:'c',2:'g',3:'t'})
nt_complement=dict({'a':'t','c':'g','g':'c','t':'a','n':'n'})

def read_sequence_from_fasta(fin, bpstart, bpend, line_length=50.0):
    bpstart=bpstart-1
    
    fin.seek(0)
    fin.readline()  #read the first line; the pointer is at the second line

    nbp = bpend - bpstart
    offset = int( bpstart + math.floor(bpstart/line_length)) #assuming each line contains 50 characters; add 1 offset per line
    
    if offset > 0:
        fin.seek(int(offset),1)

    seq = fin.read(nbp+int(math.floor(nbp/line_length))+1)
    seq = seq.replace('\n','')

    if len(seq) < nbp: 
        print 'Coordinate out of range:',bpstart,bpend
    
    return seq[0:nbp].lower()

def FastaIterator(handle):

    while True:
        line = handle.readline()
        if line == "" : return 
        if line[0] == ">": #skip >
            break

    while True:
        if line[0]!=">":
            raise ValueError("Fasta files should start with '>'")
        else:
            descr = line[1:].rstrip()
            id = descr.split()[0]
            name = id

        lines = []
        line = handle.readline()
        while True:
            if not line : break
            if line[0] == ">": break
            lines.append(line.rstrip().replace(" ","").replace("\r",""))
            line = handle.readline()

        yield "".join(lines)

        if not line : return #End

class Coordinate:
    
    def chr_id2_ord(self):
        id=self.chr_id.replace('chr','')
        try:
            id=int(id)
        except:
            cs=0
            for c in id:
                cs+=ord(c)
            id=cs
        
        return id
    
    
    def __init__(self,chr_id,bpstart,bpend,name=None,score=None,strand=None):
        self.chr_id=chr_id
        self.bpstart=bpstart
        self.bpend=bpend
        self.name=name
        self.score=score
        self.strand=strand


    def bpcenter(self):
        return (self.bpstart+self.bpend)/2

    def __eq__(self,other):
        return (self.chr_id==other.chr_id) & (self.bpstart==other.bpstart) & (self.bpend==other.bpend)
    
    def __ne__(self,other):
        return not self.__eq__(other)
    
    def __lt__(self, other):
        if self.chr_id2_ord<other.chr_id2_ord:
            return True 
        elif self.chr_id2_ord>other.chr_id2_ord:
            return False
        elif  self.bpstart<other.bpstart:
            return True
        elif self.bpstart>other.bpstart:
            return False
        else: 
            return False
    
    def __gt__(self,other):
        return not ( self.__lt__(other) or  self.__eq__(other)) 
    
    def __le__(self, other):
        return self.__eq__(other) or self.__lt__(other)
    
    def __ge__(self, other):
        return self.__eq__(other) or self.__gt__(other)
            
    def __hash__(self):
        return hash((self.chr_id,self.bpstart,self.bpend))

    def __str__(self):
        return self.chr_id+':'+str(self.bpstart)+'-'+str(self.bpend)+ (' '+self.name if self.name else '')  + ( ' '+  str(self.score) if self.score else '')+ (' '+self.strand if self.strand else '')
    
    def __repr__(self):
        return self.chr_id+':'+str(self.bpstart)+'-'+str(self.bpend)+ (' '+self.name if self.name else '')  + ( ' '+  str(self.score) if self.score else '')+ (' '+self.strand if self.strand else '')
    
    def __len__(self):
        return self.bpend-self.bpstart
    
    def __and__(self,other):
        if self.bpend < other.bpstart or other.bpend < self.bpstart or self.chr_id != other.chr_id:
            return None
        else:
            return Coordinate(self.chr_id,max(self.bpstart,other.bpstart),min(self.bpend,other.bpend))

    def upstream(self,offset=2000):
            return Coordinate(self.chr_id,self.bpend+1,self.bpend+offset)
    
    def downstream(self,offset=2000):
        return Coordinate(self.chr_id,max(0,self.bpstart-offset),self.bpstart-1)
    
       
    @classmethod
    def bed_to_coordinates(cls,bed_filename,header_lines=0, cl_chr_id=0, cl_bpstart=1, cl_bpend=2, cl_name=3, cl_score=4, cl_strand=5):
        with open(bed_filename,'r') as infile:
            coordinates = list()
            
            for _ in range(header_lines):
                infile.readline()
            
            for line in infile:
                line=line.strip()
                try:
                    coord=line.split('\t')
                    chr_id=coord[cl_chr_id]
                    bpstart=coord[cl_bpstart]
                    bpend=coord[cl_bpend]
                    try:
                        name=coord[cl_name]
                    except:
                        name='ND'
                    try:
                        score=float(coord[cl_score])
                    except:
                        score=0                        
                    try:
                        strand=coord[cl_strand]
                    except:
                        strand='ND'
                    coordinates.append(Coordinate(str(chr_id), int(bpstart), int(bpend), name, score,strand))
                except:
                    print 'Skipping line:',line

            return coordinates
    
    @classmethod
    def bed_to_coordinates_dict(cls,bed_file,header_lines=0,cl_chr_id=0, cl_bpstart=1, cl_bpend=2, cl_name=3, cl_score=4, cl_strand=5):
        with open(bed_file,'r') as infile:
            coordinates = dict()
            
            for _ in range(header_lines):
                infile.readline()
            
            
            for idx,line in enumerate(infile):
                coord=line.split()
                chr_id=coord[cl_chr_id]
                bpstart=coord[cl_bpstart]
                bpend=coord[cl_bpend]
                try:
                    name=coord[cl_name]
                except:
                    name='ND'
                try:
                    score=coord[cl_score]
                except:
                    score='ND'                        
                try:
                    strand=coord[cl_strand]
                except:
                    strand='ND'
                c=Coordinate(str(chr_id), int(bpstart), int(bpend), name, score,strand)
                print c
                coordinates[c]=idx+1

            return coordinates
    
    @classmethod
    def coordinates_to_bed(cls,coordinates,bed_file,minimal_format=False):
        with open(bed_file,'w+') as outfile:
            for c in coordinates:
                if minimal_format:
                    outfile.write('%s\t%d\t%d\n' %(c.chr_id,c.bpstart,c.bpend) )
                else:
                    outfile.write('%s\t%d\t%d\t%s\t%f\t%s\n' %(c.chr_id,c.bpstart,c.bpend,c.name if c.name else'ND',c.score if c.score else 0,c.strand if c.strand else '+') )
    @classmethod
    def coordinates_to_nscore_format(cls,coordinates,bed_file):
        with open(bed_file,'w+') as outfile:
            for c in coordinates:
                outfile.write('%s\t%d\n' %(c.chr_id,c.bpcenter) )
    
    @classmethod
    def coordinates_from_interval(cls,chr_id,interval):
        return Coordinate(chr_id, interval.start,interval.end,)
    
    @classmethod
    def coordinates_to_fasta(cls,coordinates,fasta_file,genome,chars_per_line=50,mask_repetitive=False):
        with open(fasta_file,'w+') as outfile:
            for c in coordinates:
                seq=genome.extract_sequence(c,mask_repetitive)
                outfile.write('>'+str(c)+'\n'+'\n'.join(chunks(seq,chars_per_line))+'\n')
    
    @classmethod
    def calculate_intersection(cls,coords1,coords2,build_matrix=False):
    
        coords_in_common=set()
        intersection_indexes=set()
        if build_matrix:
            intersection_matrix=sparse_matrix((len(coords1),len(coords2)))
        coord_to_row_index=dict()
        row_index=0 
        interval_tree=dict()
    
        #Build the interval tree on the first set of coordinates
        for c in coords1:
            if c.chr_id not in interval_tree:
                interval_tree[c.chr_id]=Intersecter()
    
            interval_tree[c.chr_id].add_interval(Interval(c.bpstart,c.bpend))
            coord_to_row_index[c]=row_index
            row_index+=1
    
        #Calculating the intersection
        #for each coordinates on the second set check intersection and fill the matrix
        for cl_index,c in enumerate(coords2):
            if interval_tree.has_key(c.chr_id):
                coords_hits=interval_tree[c.chr_id].find(c.bpstart, c.bpend)
                #coords_in_common+=coords_hits
                for coord_hit in coords_hits:
                    c_to_add=Coordinate.coordinates_from_interval(c.chr_id, coord_hit)
                    coords_in_common.add(c_to_add)
                    row_index=coord_to_row_index[c_to_add]
                    intersection_indexes.add(row_index)
                    
                    if build_matrix:
                        intersection_matrix[row_index,cl_index]+=1
        
        if build_matrix:
            return list(coords_in_common),list(intersection_indexes),intersection_matrix
        else:
            return list(coords_in_common),list(intersection_indexes)


    bpcenter=property(bpcenter)
    chr_id2_ord=property(chr_id2_ord)


    @classmethod
    def coordinates_of_intervals_around_center(cls,coords,window_size):
        half_window=window_size/2
        return [Coordinate(c.chr_id,c.bpcenter-half_window,c.bpcenter+half_window,strand=c.strand,name=c.name,score=c.score) for c in coords]

#intersecter
class Coordinates_Intersecter:
    def __init__(self,coordinates):
        self.interval_tree=dict()

        for c in coordinates:
            if c.chr_id not in self.interval_tree:
                self.interval_tree[c.chr_id]=Intersecter()

            self.interval_tree[c.chr_id].add_interval(Interval(c.bpstart,c.bpend,c))
 

    def find_intersections(self,c):
        coords_in_common=list()
        if self.interval_tree.has_key(c.chr_id):
            coords_hits=self.interval_tree[c.chr_id].find(c.bpstart-1, c.bpend+1)
            
            for coord_hit in coords_hits:
                coords_in_common.append(coord_hit.value)

        return coords_in_common

class Gene:
    
    regions=[8000,2000,0,1000]
    
    def __init__(self,access,name,coordinate,regions=None,exons=None,introns=None,):
        self.access=access
        self.c=coordinate
        self.name=name
        self.exons=exons
        self.introns=introns
        
        if regions:
            self.regions=regions
    
    
    def __str__(self):
        return self.name+'_'+self.access+'_'+str(self.c)
    
    def __repr__(self):
        return self.name+'_'+self.access+'_'+str(self.c)
    
    def tss(self):
        if self.c.strand=='-':
            return self.c.bpend
        else:
            return self.c.bpstart

    def tes(self):
        if self.c.strand=='+':
            return self.c.bpend
        else:
            return self.c.bpstart
    
    def end(self):
        if self.c.strand=='+':
            return self.c.bpend
        else:
            return self.c.bpstart
    
    def distal_c(self):
        if self.c.strand=='+':
            return Coordinate(self.c.chr_id,self.tss-self.regions[0],self.tss-self.regions[1]-1,strand='+')
        else:
            return Coordinate(self.c.chr_id,self.tss+self.regions[1]+1,self.tss+self.regions[0],strand='-') 
        
    def promoter_c(self):
        if self.c.strand=='+':
            return Coordinate(self.c.chr_id,self.tss-self.regions[1],self.tss+self.regions[2]-1,strand='+')
        else:
            return Coordinate(self.c.chr_id,self.tss-self.regions[2]+1,self.tss+self.regions[1],strand='-') 
    
    def intra_c(self):
        if self.c.strand=='+':
            return Coordinate(self.c.chr_id,self.tss+self.regions[2],self.end+self.regions[3],strand='+')
        else:
            return Coordinate(self.c.chr_id,self.end-self.regions[3],self.tss-self.regions[2],strand='-')
    
    def full_c(self):
        if self.c.strand=='+':
            return Coordinate(self.c.chr_id,self.tss-self.regions[0],self.end+self.regions[3],strand='+')
        else:
            return Coordinate(self.c.chr_id,self.end-self.regions[3],self.tss+self.regions[0],strand='-') 
        
    @classmethod    
    def load_from_annotation(cls,gene_annotation_file,load_exons_introns_info=False,header_lines=1,regions=regions,format='txt'):
        genes_list=[]
        with open(gene_annotation_file,'r') as genes_file:
            
            for _ in range(header_lines):
                genes_file.readline()
           
            if format=='txt':
                idx_chr_id=2
                idx_bpstart=4
                idx_bpend=5
                idx_strand=3
                idx_access=1
                idx_name=-4
                idx_exon_starts=9
                idx_exon_ends=10
            else:
               idx_chr_id=0
               idx_bpstart=1
               idx_bpend=2
               idx_strand=5
               idx_access=3
               idx_name=3
               idx_exon_starts=10
               idx_exon_ends=11
               
               
           
            for gene_line in genes_file:
                fields=gene_line.split('\t')
                chr_id=fields[idx_chr_id]
                bpstart=int(fields[idx_bpstart])
                bpend=int(fields[idx_bpend])
                strand=fields[idx_strand]
                access=fields[idx_access]
                name=fields[idx_name]
                c=Coordinate(chr_id,bpstart,bpend,strand=strand)

                if load_exons_introns_info:
                    exon_starts=map(int,fields[idx_exon_starts].split(',')[:-1])
                    exon_ends=map(int,fields[idx_exon_ends].split(',')[:-1])

                    exons=[Coordinate(chr_id,bpstart,bpend,strand=strand) for bpstart,bpend in zip(exon_starts,exon_ends)]
                    introns=[Coordinate(chr_id,bpstart+1,bpend-1,strand=strand) for bpstart,bpend in zip(exon_ends[:-1],exon_starts[1:])]
                    genes_list.append(Gene(access,name,c,exons=exons,introns=introns,regions=regions))
                else:
                    genes_list.append(Gene(access,name,c,regions=regions))
                
        return genes_list

    @classmethod
    def exons_from_annotations(cls,gene_annotation_file,header_lines=1):
        exons_list=[]

        with open(gene_annotation_file,'r') as genes_file:

            for _ in range(header_lines):
                genes_file.readline()            
            
            for gene_line in genes_file:
                fields=gene_line.split('\t')
                chr_id=fields[2]
                strand=fields[3]
                access=fields[1]
                exon_starts=map(int,fields[9].split(',')[:-1])
                exon_ends=map(int,fields[10].split(',')[:-1])
                exons=[Coordinate(chr_id,bpstart,bpend,strand=strand,name=access) for bpstart,bpend in zip(exon_starts,exon_ends)]
                exons_list+=exons
                
            return exons_list

        
    @classmethod
    def introns_from_annotations(cls,gene_annotation_file,header_lines=1):
        introns_list=[]

        with open(gene_annotation_file,'r') as genes_file:
            
            for _ in range(header_lines):
                genes_file.readline()            
            
            for gene_line in genes_file:
                fields=gene_line.split('\t')
                chr_id=fields[2]
                strand=fields[3]
                access=fields[1]
                exon_starts=map(int,fields[9].split(',')[:-1])
                exon_ends=map(int,fields[10].split(',')[:-1])
                introns=[Coordinate(chr_id,bpstart-1,bpend,strand=strand,name=access) for bpstart,bpend in zip(exon_ends[:-1],exon_starts[1:])]
                introns_list+=introns
        
            return introns_list


    @classmethod
    def genes_coordinates_from_annotations(cls,gene_annotation_file,header_lines=1):
        genes_coordinates=[]
        with open(gene_annotation_file,'r') as genes_file:
            
            for _ in range(header_lines):
                genes_file.readline()            
            
            for gene_line in genes_file:
                fields=gene_line.split('\t')
                chr_id=fields[2]
                bpstart=int(fields[4])
                bpend=int(fields[5])
                strand=fields[3]
                access=fields[1]
                name=fields[-4]

                genes_coordinates.append(Coordinate(chr_id,bpstart,bpend,strand=strand,name=access))


            return genes_coordinates
    
    
    @classmethod
    def promoter_coordinates_from_annotations(cls,gene_annotation_file,load_exons_introns_info=False,header_lines=1,promoter_region=None):
        if promoter_region:
            return [g.promoter_c for g in cls.load_from_annotation(gene_annotation_file,load_exons_introns_info=False,header_lines=1,regions=[Gene.regions[0],promoter_region[0],promoter_region[1],Gene.regions[3]])]
        else:
            return [g.promoter_c for g in cls.load_from_annotation(gene_annotation_file,load_exons_introns_info=False,header_lines=1)]
            
    
    @classmethod
    def write_genomic_regions_bed(cls,gene_annotation_file,genome_name='',minimal_format=True, promoter_region=None):
        
        exons=Gene.exons_from_annotations(gene_annotation_file)
        Coordinate.coordinates_to_bed(exons,genome_name+'exons.bed',minimal_format=minimal_format)
        del(exons)
        
        introns=Gene.introns_from_annotations(gene_annotation_file)
        Coordinate.coordinates_to_bed(introns,genome_name+'introns.bed',minimal_format=minimal_format)
        del(introns)

        promoters=Gene.promoter_coordinates_from_annotations(gene_annotation_file,promoter_region=promoter_region)
        Coordinate.coordinates_to_bed(promoters,genome_name+'promoters.bed',minimal_format=True)
        del(promoters)
 
       
    tss=property(tss)
    tes=property(tes)
    distal_c=property(distal_c)
    promoter_c=property(promoter_c)
    intra_c=property(intra_c)
    full_c=property(full_c)

class Sequence:
    
    def __init__(self,seq=''):
        self.seq=string.lower(seq)

    def set_bg_model(self,ACGT_probabilities):
        self.bg_model= rv_discrete(name='bg', values=([0, 1, 2,3], ACGT_probabilities))
 
    def __str__(self):
        return self.seq
    
    def __len__(self):
        return len(self.seq)
   
    # seq_complement
    def _reverse_complement(self):
        return "".join([nt_complement[c] for c in self.seq[-1::-1]])

    reverse_complement=property(_reverse_complement)
    
    @classmethod
    def reverse_complement(self,seq):
        return "".join([nt_complement[c] for c in seq[-1::-1]])
    
    @classmethod
    def generate_random(cls,n,bgmodel=rv_discrete(name='bg', values=([0, 1, 2,3], [0.2955, 0.2045, 0.2045,0.2955]))):
        int_seq=cls.bg_model.rvs(size=n)
        return ''.join([int2nt[c] for c in int_seq])
    
class Seq_set(dict):
    def add_name(self,name):
        self.name=name
    def add_seq(self,seq,cord):
        self[cord]=seq

    def remove_seq_from_cord(self,cord):
        if cord in self.keys():
            del self[cord]
        else:
            print('sequence not found')

class Genome:
    def __init__(self,genome_directory,release='ND',verbose=False):
        self.chr=dict()
        self.genome_directory=genome_directory
        self.release=release
        self.chr_len=dict()
        self.verbose=verbose
        
        for infile in glob.glob( os.path.join(genome_directory, '*.fa') ):
            try:

                chr_id=os.path.basename(infile).replace('.fa','')
                self.chr[chr_id] = open(infile,'r')
                
                self.chr_len[chr_id]=0
                
                self.chr[chr_id].readline()
                for line in self.chr[chr_id]:
                    self.chr_len[chr_id]+=len(line.strip())
                
                if verbose:    
                    print 'Read:'+ infile
            except:
                if verbose:
                    print 'Error, not loaded:',infile
        
        if verbose:
            print 'Genome initializated'
            
    def estimate_background(self):
        counting={'a':.0,'c':.0,'g':.0,'t':.0}
        all=0.0
        
        for chr_id in self.chr.keys():
            if self.verbose:
                print 'Counting on:',chr_id
            
            self.chr[chr_id].seek(0)
            self.chr[chr_id].readline()
            
            for line in self.chr[chr_id]:
                for nt in counting.keys():
                    count_nt=line.lower().count(nt)
                    counting[nt]+=count_nt
                    all+=count_nt
        
        if self.verbose:
            print counting
        
        for nt in counting.keys():
            counting[nt]/=all
            
        return counting

    
    def extract_sequence(self,coordinate, mask_repetitive=False,line_length=50.0):
        if not self.chr.has_key(coordinate.chr_id):
            if self.verbose:
                print "Warning: chromosome %s not present in the genome" % coordinate.chr_id
        else:

            bpstart=  coordinate.bpstart-1
            bpend=coordinate.bpend
            
            self.chr[coordinate.chr_id].seek(0)
            self.chr[coordinate.chr_id].readline()
            
           
            nbp = bpend - bpstart
            offset = int( bpstart + math.floor(bpstart/line_length))-1 

            if offset > 0:
                self.chr[coordinate.chr_id].seek(offset,1)
            
            seq = self.chr[coordinate.chr_id].read(nbp+int(math.floor(nbp/line_length))+1)
            seq = seq.replace('\n','')
            
            if len(seq) < nbp: 
                if self.verbose:
                    print 'Warning: coordinate out of range:',bpstart,bpend
            
            if mask_repetitive:
                seq=  ''.join([mask(c) for c in  seq[0:nbp]]).lower()
            else:
                seq=seq[0:nbp].lower()
            
            if coordinate.strand=='-':
                return Sequence.reverse_complement(seq).lower()
            else:
                return seq        

class Genome_mm:
    
    def __init__(self,genome_directory,release='ND',verbose=False):
        self.chr=dict()
        self.genome_directory=genome_directory
        self.release=release        
        self.chr_len=dict()
        self.verbose=verbose

        for infile in glob.glob( os.path.join(genome_directory, '*.fa') ):
            mm_filename=infile.replace('.fa','.mm')
            
            filename=infile.replace(genome_directory,'').replace('.fa','')
            chr_id=os.path.basename(infile).replace('.fa','')

            if not os.path.isfile(mm_filename):
                if verbose:
                    print 'Missing:'+chr_id+' generating memory mapped file (This is necessary only the first time) \n'
                with open(infile) as fi:
                    with open(os.path.join(genome_directory,chr_id+'.mm'),'w+') as fo:
                        #skip header
                        fi.readline()
                        for line in fi:
                            fo.write(line.strip()) 
                    if verbose:
                        print 'Memory mapped file generated for:',chr_id 

            with open(mm_filename,'r') as f:
                self.chr[chr_id]= mmap.mmap(f.fileno(),0, access=mmap.ACCESS_READ) 
                self.chr_len[chr_id]=len(self.chr[chr_id])
                if verbose:
                    print "Chromosome:%s Read"% chr_id
    
        if verbose:
            print 'Genome initializated'
        
        
    def extract_sequence(self,coordinate,mask_repetitive=False):
        if mask_repetitive:
            seq= ''.join([mask(c) for c in self.chr[coordinate.chr_id][coordinate.bpstart-1:coordinate.bpend]]).lower()
        else:
            seq= self.chr[coordinate.chr_id][coordinate.bpstart-1:coordinate.bpend].lower()
        
        if coordinate.strand=='-':
            return Sequence.reverse_complement(seq).lower()
        else:
            return seq
    

    def estimate_background(self):
        counting={'a':.0,'c':.0,'g':.0,'t':.0}
        all=0.0
        for chr_id in self.chr.keys():
            if self.verbose:
                print 'Counting on:',chr_id

            
            for nt in counting.keys():
                
                count_nt=self.chr[chr_id][:].lower().count(nt)
                counting[nt]+=count_nt
                all+=count_nt
        
        if self.verbose:
            print counting


        for nt in counting.keys():
            counting[nt]/=all
        
        return counting

class Genome_2bit:

    def __init__(self,genome_2bit_file,verbose=False):
        self.genome=TwoBitFile(open(genome_2bit_file,'rb'))
        self.chr_len=dict()
        self.verbose=verbose

        for chr_id in self.genome.keys():
            self.chr_len[chr_id]=len(self.genome[chr_id])
        if verbose:
            print 'Genome initializated'
        
    def extract_sequence(self,coordinate,mask_repetitive=False):
        if mask_repetitive:
            seq= ''.join([mask(c) for c in self.genome[coordinate.chr_id][coordinate.bpstart-1:coordinate.bpend]]).lower()
        else:
            seq= self.genome[coordinate.chr_id][coordinate.bpstart-1:coordinate.bpend].lower()
        
        if coordinate.strand=='-':
            return Sequence.reverse_complement(seq)
        else:
            return seq
    
    def estimate_background(self):
        counting={'a':.0,'c':.0,'g':.0,'t':.0}
        all=0.0
        
        for chr_id in self.genome.keys():
            if self.verbose:
                start_time = time.time()
                print 'Counting on:',chr_id

            for nt in counting.keys():
                
                count_nt=self.genome[chr_id][:].lower().count(nt)
                counting[nt]+=count_nt
                all+=count_nt

            print 'elapsed:',time.time() - start_time
        
        if self.verbose:
            print counting

        for nt in counting.keys():
            counting[nt]/=all
        
        return counting


    def write_meme_background(self,filename):
        counting=self.estimate_background()
        with open(filename,'w+') as outfile:
            for nt in counting.keys():
                outfile.write('%s\t%2.4f\n' % (nt,counting[nt]))

    def write_chr_len(self,filename):
        with open(filename,'w+') as outfile:
            for chr_id in self.genome.keys():
                outfile.write('%s\t%s\n' % (chr_id,self.chr_len[chr_id]) )


class Fimo:
    def __init__(self,meme_motifs_filename, bg_filename,p_value=1.e-4,temp_directory='./'):
        #be aware that they have changed the command line interface recently!
        self.fimo_command= 'fimo --text --thresh '+str(p_value)+'  --bgfile '+bg_filename+' '+meme_motifs_filename 
        self.temp_directory=temp_directory
        
        with open(meme_motifs_filename) as infile:
             self.motif_id_to_name=dict()
             self.motif_id_to_index=dict()
             self.motif_names=[]
             self.motif_name_to_index=dict()
             self.motif_ids=[]
             motif_index=0
             for line in infile:
                 try:
                     if 'MOTIF' in line:
 
                         #in meme format sometime you don't have the name and id but only one string for both
                         #like in jolma 2013...
                         motif_id=line.split()[1]
 
                         try:
                             motif_name=line.split()[2]
                         except:
                             motif_name=motif_id
                             
                         self.motif_id_to_name[motif_id]=motif_name
                         self.motif_id_to_index[motif_id]=motif_index
                         self.motif_name_to_index[motif_name]=motif_name
                         self.motif_ids.append(motif_id)
                         self.motif_names.append(motif_name)
                         motif_index+=1
                 except:
                     print 'problem with this line:', line

        
    def extract_motifs(self,seq, report_mode='full'):
        if report_mode=='indexes_set':
                motifs_in_sequence=set()
        elif report_mode=='fq_array':
            motifs_in_sequence=np.zeros(len(self.motif_names))
        elif report_mode=='full':
            motifs_in_sequence=list()
        elif report_mode=='fq_and_presence':
            motifs_in_sequence={'presence':set(),'fq':np.zeros(len(self.motif_names))}
        else:
            raise Exception('report_mode not recognized')
            
        
        with tempfile.NamedTemporaryFile('w+',dir=self.temp_directory,delete=False) as tmp_file:
            tmp_file.write(''.join(['>S\n',seq,'\n']))
            tmp_filename=tmp_file.name
            tmp_file.close()
            
            fimo_process=subprocess.Popen(self.fimo_command+' '+tmp_filename,stdin=None,stdout=subprocess.PIPE,stderr=subprocess.PIPE,shell=True)
            output=fimo_process.communicate()[0]
            fimo_process.wait()
            
            os.remove(tmp_filename)
            
            lines=output.split('\n')
            lines=lines[1:]
            for line in lines:
                if line:
                    fields=line.split('\t')
                    motif_id=fields[0]
                    motif_name=self.motif_id_to_name[motif_id]
                    
                    if report_mode=='full':
                        c_start=int(fields[2])
                        c_end=int(fields[3])
                        strand=fields[4]
                        score=float(fields[5])
                        p_value=float(fields[6])
                        length=len(fields[8])
                        
                     
                        motifs_in_sequence.append({'id':motif_id,'name':motif_name,'start':c_start,'end':c_end,'strand':strand,'score':score,'p_value':p_value,'length':length})
                    elif report_mode=='indexes_set':
                        motifs_in_sequence.add(self.motif_name_to_index[motif_name])
                    
                    elif report_mode=='fq_array':
                        motifs_in_sequence[self.motif_name_to_index[motif_name]]+=1
                    
                    elif report_mode=='fq_and_presence':
                        motifs_in_sequence['presence'].add(self.motif_name_to_index[motif_name])
                        motifs_in_sequence['fq'][self.motif_name_to_index[motif_name]]+=1
                    
 
            return motifs_in_sequence if report_mode=='fq_array' or 'fq_and_presence' else list(motifs_in_sequence)

def build_motif_in_seq_matrix(bed_filename,genome_directory,meme_motifs_filename,bg_filename,genome_mm=True,temp_directory='./',mask_repetitive=False,p_value=1.e-4, check_only_presence=False):

    print 'Loading coordinates  from bed'
    target_coords=Coordinate.bed_to_coordinates(bed_filename)

    print 'Initialize Genome'
    if genome_mm:
        genome=Genome_mm(genome_directory)
    else:
        genome=Genome(genome_directory)

    print 'Initilize Fimo and load motifs'
    fimo=Fimo(meme_motifs_filename,bg_filename,temp_directory=temp_directory,p_value=p_value)

    print 'Initialize the matrix'
    motifs_in_sequences_matrix=np.zeros((len(target_coords),len(fimo.motif_names)))

    for idx_seq,c in enumerate(target_coords):
        seq=genome.extract_sequence(c,mask_repetitive)
        print idx_seq, len(target_coords)
        if check_only_presence:
            motifs_in_sequences_matrix[idx_seq,fimo.extract_motifs(seq,report_mode='indexes_set')]=1
        else:
            motifs_in_sequences_matrix[idx_seq,:]+=fimo.extract_motifs(seq,report_mode='fq_array')
            

    return motifs_in_sequences_matrix, fimo.motif_names, fimo.motif_ids

def build_motif_profile(target_coords,genome,meme_motifs_filename,bg_filename,genome_mm=True,temp_directory='./',mask_repetitive=False,p_value=1.e-4,check_only_presence=False):


    #print 'Initilize Fimo and load motifs'
    fimo=Fimo(meme_motifs_filename,bg_filename,temp_directory=temp_directory,p_value=p_value)

    #print 'Allocate memory'
    if check_only_presence:
        motifs_in_sequences_profile=np.zeros(len(fimo.motif_names))
    else:
        motifs_in_sequences_profile={'fq':np.zeros((len(target_coords),len(fimo.motif_names))),'presence':np.zeros(len(fimo.motif_names))}
    for idx_seq,c in enumerate(target_coords):
        seq=genome.extract_sequence(c,mask_repetitive)
        print idx_seq, len(target_coords)
        if check_only_presence:
            motifs_in_sequences_profile[fimo.extract_motifs(seq,report_mode='indexes_set')]+=1
        else:
            motifs_in_sequences=fimo.extract_motifs(seq,report_mode='fq_and_presence')
            #print motifs_in_sequences,motifs_in_sequences['presence'],motifs_in_sequences['fq']
            motifs_in_sequences_profile['presence'][list(motifs_in_sequences['presence'])]+=1
            motifs_in_sequences_profile['fq'][idx_seq,:]+=motifs_in_sequences['fq']

    return motifs_in_sequences_profile, fimo.motif_names, fimo.motif_ids

'''
given a set of coordinates in a bed file and a bam/sam file calculate the profile matrix
'''
def calculate_profile_matrix_bed_bam(bed_filename,sam_filename,window_size=5000, resolution=50, fragment_length=200, use_strand=False,return_coordinates=False):
    cs=Coordinate.bed_to_coordinates(bed_filename)
    cs=Coordinate.coordinates_of_intervals_around_center(cs, window_size)
    samfile = pysam.Samfile(sam_filename)
    n_bins=(window_size/resolution)+1
    
    profile_matrix=np.zeros((len(cs),n_bins))

    for idx_c,c in enumerate(cs):
        n_start=max(0,c.bpcenter-window_size/2)
        n_end=c.bpcenter+window_size/2

        for rd in samfile.fetch(c.chr_id, n_start,n_end):

            if rd.is_reverse:
                rd_st=rd.positions[-1]-fragment_length-1
                rd_end=rd.positions[-1]
            else:
                rd_st=rd.positions[0]
                rd_end=max(rd.positions[-1],rd.positions[1]+fragment_length)
             
            bin_idx_st=max(0,1+(rd_st-n_start)/resolution)
            bin_idx_en=min(n_bins,1+ (rd_end-n_start)/resolution)

            #print rd.positions[1],rd.positions[-1],rd_st, rd_end,rd_end-rd_st,rd_st-n_start,rd_end-n_start,bin_idx_st,bin_idx_en
            
            profile_matrix[idx_c,bin_idx_st:bin_idx_en]+=1
        
        if use_strand and c.strand == '-':
            profile_matrix[idx_c,:]=profile_matrix[idx_c,::-1]
            
    if return_coordinates:
        return profile_matrix,samfile.mapped,cs
    else:    
        return profile_matrix,samfile.mapped

def extract_bg_from_bed(bed_filename,genome_directory,bg_filename,genome_mm=True):
    
    acgt_fq={'a':0.0,'c':0.0,'g':0.0,'t':0.0}
    total=0

    print 'Loading coordinates  from bed'
    target_coords=Coordinate.bed_to_coordinates(bed_filename)

    print 'Initialize Genome'
    if genome_mm:
        genome=Genome_mm(genome_directory)
    else:
        genome=Genome(genome_directory)
    
    for idx_seq,c in enumerate(target_coords):
        seq=genome.extract_sequence(c)
        for nt in ['a','c','t','g']:
            acgt_fq[nt]+=seq.count(nt)
        
    total=sum(acgt_fq.values())

    
    for nt in ['a','c','t','g']:
        acgt_fq[nt]/=total

          
    print acgt_fq
    with open(bg_filename, 'w+') as out_file:
        for nt in ['a','c','t','g']:
            out_file.write('%s\t%1.4f\n' % (nt,acgt_fq[nt]))

#hgWiggle wrapper
def read_from_wig(c,wig_path,wig_mask='.phastCons44way.hg18.compiled',only_average=False):
    position=c.chr_id+':'+str(c.bpstart)+'-'+str(c.bpend)
    wig_file=os.path.join(wig_path,c.chr_id+wig_mask)

    if only_average:
        command=' '.join(['hgWiggle','-position='+position,wig_file,"-doStats | sed  -e '1,3d' | cut -f4,10"])
    else:
        command=' '.join(['hgWiggle','-position='+position,wig_file," -rawDataOut "])

    wig_process=subprocess.Popen(command,stdin=None,stdout=subprocess.PIPE,stderr=subprocess.PIPE,shell=True)
    output=wig_process.communicate()[0]

    if only_average:    
        return tuple(output.split())
    else:
        values=output.split()
        return len(values),values

class Annotator:
    
    def __init__(self,input_filename,annotations_filenames,annotation_names=None):

        self.annotations_filenames=annotations_filenames
        self.input_filename=input_filename

        print annotation_names
        
        #check if we have custom names defined
        if annotation_names is None:
            self.annotation_names=annotations_filenames
        else:
            self.annotation_names=annotation_names

        assert len(annotations_filenames) == len(annotation_names)

        #associate to each name a prime number for the multiple annotation trick..
        self.annotation_names_to_prime={name:prime for (name,prime) in zip(self.annotation_names,self.__primes(len(self.annotation_names)))}
        print self.annotation_names_to_prime

        self.input_coordinates=Coordinate.bed_to_coordinates(input_filename)

    def annotate(self):
        #allocate_memory
        self.annotation_track=np.ones(len(self.input_coordinates),dtype=np.int)

        self.interval_tree=dict()
        self.coord_to_row_index=dict()
        self.row_index=0 

        #Build the interval Tree
        for c in self.input_coordinates:
            if c.chr_id not in self.interval_tree:
                self.interval_tree[c.chr_id]=Intersecter()
    
            self.interval_tree[c.chr_id].add_interval(Interval(c.bpstart,c.bpend))
            self.coord_to_row_index[c]=self.row_index
            self.row_index+=1

        for idx,bed_filename in enumerate(self.annotations_filenames):
            coordinates=Coordinate.bed_to_coordinates(bed_filename)

            prime_number=self.annotation_names_to_prime[self.annotation_names[idx]]
            for idx_intersection in  self.__intersection_indexes(coordinates):
                self.annotation_track[idx_intersection]*=prime_number
        
    def coordinates_by_annotation_name(self,annotation_name):
        if annotation_name=='out':
            return [self.input_coordinates[idx] for idx,value in enumerate(self.annotation_track) if value ==1]
        else:
            prime_number=self.annotation_names_to_prime[annotation_name]
            return [self.input_coordinates[idx] for idx,value in enumerate(self.annotation_track) if (value % prime_number)==0]

    def save_annotation_track_matlab(self,filename):
        savemat(filename,{'annotation_track':self.annotation_track,'mapping':self.annotation_names_to_prime})
        print 'Annotation track saved to:',filename

    def save_annotation_track_bed(self,filename):
        with open(filename,'w+') as outfile:

            for idx,c in enumerate(self.input_coordinates):
                annotated=False

                line='%s\t%s\t%d\t' % (c.chr_id, c.bpstart, c.bpend,)
                for name in self.annotation_names_to_prime.keys():
                    if self.annotation_track[idx] % self.annotation_names_to_prime[name] == 0:
                        line+=name+','
                        annotated=True

                line=line[:-1]
                outfile.write(line+'\n')

        print 'Annotation track saved to:',filename
    
    def __intersection_indexes(self,coordinates):
        intersection_indexes=set()

        for cl_index,c in enumerate(coordinates):
            if self.interval_tree.has_key(c.chr_id):

                coords_hits=self.interval_tree[c.chr_id].find(c.bpstart, c.bpend)
                for coord_hit in coords_hits:
                    intersection_indexes.add(self.coord_to_row_index[Coordinate.coordinates_from_interval(c.chr_id, coord_hit)])

        return intersection_indexes

    def __gen_primes(self):

        D = {}  
        q = 2  

        while True:
            if q not in D:
                yield q        
                D[q * q] = [q]
            else:

                for p in D[q]:
                    D.setdefault(p + q, []).append(p)
                del D[q]

            q += 1

    def __primes(self,n):
        primes=self.__gen_primes()
        return [primes.next() for i in range(n)]

class Ngram:
    def __init__(self,ngram_length=4,alphabet_size=4):
        #print 'Ngram initialization'
        #build useful dictionary
        self.alphabet_size=alphabet_size
        self.ngram_length=ngram_length
        
        all_ngram=map(self.int2ngram,range(self.alphabet_size**self.ngram_length),[self.ngram_length]*(self.alphabet_size**self.ngram_length),[self.alphabet_size]*(self.alphabet_size**self.ngram_length))
        self.ngram2index=dict()
        self.index2ngram=dict()
        self.non_redundant_ngram2index=dict()
        self.non_redundant_index2ngram=dict()
        self.ngram_rev_complement=dict()
        self.number_of_ngrams=len(all_ngram)    
        
        for idx,ngram in enumerate(all_ngram):
            self.ngram2index[ngram]=idx
            self.ngram_rev_complement[ngram]=Sequence.reverse_complement(ngram)
            self.index2ngram[idx]=ngram
        
        
        for idx,ngram in enumerate([all_ngram[i] for i in self.non_redundant_idx()]):
            self.non_redundant_ngram2index[ngram]=idx
            self.non_redundant_index2ngram[idx]=ngram
        
        self.number_of_non_redundant_ngrams=len(self.non_redundant_ngram2index)
                
    def int2ngram(self,idx,ngram_length,alphabet_size):
        l=[]
        for _ in range(ngram_length):
            l.append(int2nt[idx % alphabet_size])
            idx/=alphabet_size

        return "".join(l)[-1::-1]
    
    def non_redundant_idx(self):
        ngram_taken=set()
        non_redundant_idxs=[]
        for idx in range(self.number_of_ngrams):
            n=self.index2ngram[idx]
            if (self.ngram2index[n] in ngram_taken) or (self.ngram2index[self.ngram_rev_complement[n]] in ngram_taken ):
                pass
            else:
                non_redundant_idxs.append(idx)
                ngram_taken.add(self.ngram2index[n])
                ngram_taken.add(self.ngram2index[self.ngram_rev_complement[n]])
        
        return tuple(non_redundant_idxs)
    
    def build_ngram_fq_vector(self,seq):
   
        ngram_vector=zeros(self.number_of_ngrams)
   
        for i in xrange(len(seq)-self.ngram_length+1):
            try:
                ngram_vector[self.ngram2index[seq[i:i+self.ngram_length]]]+=1
            except:
                pass
            #ngram_vector[self.ngram2index[self.ngram_rev_complement[seq[i:i+self.ngram_length]]]]+=1
        
        return ngram_vector
    
    def build_ngram_fq_vector_non_redundant(self,seq):
   
        ngram_vector=zeros(self.number_of_non_redundant_ngrams)
   
        for i in xrange(len(seq)-self.ngram_length+1):
            
            try:
                ngram_vector[self.non_redundant_ngram2index[seq[i:i+self.ngram_length]]]+=1
            except:
                pass
            try:
                ngram_vector[self.non_redundant_ngram2index[self.ngram_rev_complement[seq[i:i+self.ngram_length]]]]+=1
            except:
                pass
        
        return ngram_vector
    
    def build_ngrams_fq_matrix(self,seq_set,non_redundant=True):
        if non_redundant:
            ngram_matrix=zeros((len(seq_set),self.number_of_non_redundant_ngrams))
            for idx_seq,seq in enumerate(seq_set):
                ngram_matrix[idx_seq,:]=self.build_ngram_fq_vector_non_redundant(seq)
        else:
            ngram_matrix=zeros((len(seq_set),self.number_of_ngrams))
            for idx_seq,seq in enumerate(seq_set):
                ngram_matrix[idx_seq,:]=self.build_ngram_fq_vector(seq)
        
        return ngram_matrix,np.array([len(seq) for seq in seq_set],ndmin=2)

    def build_ngram_profile_vector_non_redundant(self,seq):

        profile=zeros(len(seq)-self.ngram_length+1)

        for i in xrange(len(seq)-self.ngram_length+1):
            
            try:
                profile[i]=self.non_redundant_ngram2index[seq[i:i+self.ngram_length]]
            except:
                pass
            try:
                profile[i]=self.non_redundant_ngram2index[self.ngram_rev_complement[seq[i:i+self.ngram_length]]]
            except:
                pass
        
        return profile


    def build_ngrams_profile_matrix_non_redundant(self,seq_set):
        profile_matrix=zeros((len(seq_set),len(seq_set[0])-self.ngram_length+1))
        for idx_seq,seq in enumerate(seq_set):
                profile_matrix[idx_seq,:]=self.build_ngram_profile_vector_non_redundant(seq)

        return profile_matrix

    
    def save_to_file(self,filename=None):
        if not filename:
            filename='ng'+str(self.ngram_length)
        with open(filename,'wb+') as outfile:
            print 'saving...'
            cPickle.dump(self, outfile,2)
            print 'done'
        
    @classmethod
    def load_from_file(cls,filename):
        with open(filename,'rb') as infile:
            return cPickle.load(infile)

