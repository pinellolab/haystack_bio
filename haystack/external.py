#from https://github.com/nfusi/qvalue
import scipy as sp
import sys
import scipy.interpolate
import  re, math,  tempfile


def estimate_qvalues(pv, m = None, verbose = False, lowmem = False, pi0 = None):
    """
    Estimates q-values from p-values

    Args
    =====

    m: number of tests. If not specified m = pv.size
    verbose: print verbose messages? (default False)
    lowmem: use memory-efficient in-place algorithm
    pi0: if None, it's estimated as suggested in Storey and Tibshirani, 2003. 
         For most GWAS this is not necessary, since pi0 is extremely likely to be
         1

    """
    
    
    assert(pv.min() >= 0 and pv.max() <= 1), "p-values should be between 0 and 1"

    original_shape = pv.shape
    pv = pv.ravel() # flattens the array in place, more efficient than flatten() 

    if m == None:
        m = float(len(pv))
    else:
        # the user has supplied an m
        m *= 1.0

    # if the number of hypotheses is small, just set pi0 to 1
    if len(pv) < 100 and pi0 == None:
        pi0 = 1.0
    elif pi0 != None:
        pi0 = pi0
    else:
        # evaluate pi0 for different lambdas
        pi0 = []
        lam = sp.arange(0, 0.90, 0.01)
        counts = sp.array([(pv > i).sum() for i in sp.arange(0, 0.9, 0.01)])
        
        for l in range(len(lam)):
            pi0.append(counts[l]/(m*(1-lam[l])))

        pi0 = sp.array(pi0)

        # fit natural cubic spline
        tck = sp.interpolate.splrep(lam, pi0, k = 3)
        pi0 = sp.interpolate.splev(lam[-1], tck)
        
        if pi0 > 1:
            if verbose:
                print("got pi0 > 1 (%.3f) while estimating qvalues, setting it to 1" % pi0)
            
            pi0 = 1.0

    assert(pi0 >= 0 and pi0 <= 1), "pi0 is not between 0 and 1: %f" % pi0


    if lowmem:
        # low memory version, only uses 1 pv and 1 qv matrices
        qv = sp.zeros((len(pv),))
        last_pv = pv.argmax()
        qv[last_pv] = (pi0*pv[last_pv]*m)/float(m)
        pv[last_pv] = -sp.inf
        prev_qv = last_pv
        for i in xrange(int(len(pv))-2, -1, -1):
            cur_max = pv.argmax()
            qv_i = (pi0*m*pv[cur_max]/float(i+1))
            pv[cur_max] = -sp.inf
            qv_i1 = prev_qv
            qv[cur_max] = min(qv_i, qv_i1)
            prev_qv = qv[cur_max]

    else:
        p_ordered = sp.argsort(pv)    
        pv = pv[p_ordered]
        qv = pi0 * m/len(pv) * pv
        qv[-1] = min(qv[-1],1.0)

        for i in xrange(len(pv)-2, -1, -1):
            qv[i] = min(pi0*m*pv[i]/(i+1.0), qv[i+1])
        
        # reorder qvalues
        qv_temp = qv.copy()
        qv = sp.zeros_like(qv)
        qv[p_ordered] = qv_temp

        # reshape qvalues
        qv = qv.reshape(original_shape)
        
    return qv


"""
FUNCTIONS FROM MotifTools.py -- Kitchen Sink Motif Objects and Operations
Copyright (2005) Whitehead Institute for Biomedical Research
All Rights Reserved
Author: David Benjamin Gordon
"""
one2two = {  'W':'AT',    'M':'AC',   'R':'AG',
             'S':'CG',    'Y':'CT',   'K':'GT'}
two2one = { 'AT': 'W',   'AC': 'M',  'AG': 'R',
            'CG': 'S',   'CT': 'Y',  'GT': 'K'}
ACGT = list('ACGT')

def Motif_from_counts(countmat,beta=0.01,bg={'A':.25,'C':.25,'G':.25,'T':.25}):
    m = Motif('',bg)
    m.compute_from_counts(countmat,beta)
    return m

class Motif:
    """
    CORE: Motif()   A pssm model, with scanning, storing, loading, and other operations
    
    There are a large number of functions and member fucntions here.  To get started,
    a motif can be instantiated by providing an ambiguity code, a set of aligned DNA sequences,
    or from matrices of counts, probabilities or log-likelihoods (aka PSSMs).
    
    >>> m =  MotifTools.Motif_from_text('TGAAACANNSYWT')
    >>> print m.oneletter()
    TGAAACA..sywT

    See code (or documentation) for details
    """
    def __init__(self,list_of_seqs_or_text=[],backgroundD=None):
        self.MAP       = 0
        self.evalue    = None
        self.oneletter = ''
        self.nseqs     = 0
        self.counts    = []
        self.width     = 0
        self.fracs     = []
        self.logP      = []
        self.ll        = []
        self.bits      = []
        self.totalbits = 0
        self.maxscore  = 0
        self.minscore  = 0
        self.pvalue      = 1
        self.pvalue_rank = 1
        self.church      = None
        self.church_rank = 1
        self.Cpvalue     = 1
        self.Cpvalue_rank= 1
        self.Cchurch     = 1
        self.Cchurch_rank= 1
        self.binomial    = None
        self.binomial_rank=1
        self.E_seq       = None
        self.frac        = None
        self.E_site      = None
        self.E_chi2      = None
        self.kellis      = None
        self.MNCP        = None
        self.ROC_auc     = None
        self.realpvalue  = None
        self.Cfrac       = None
        self.CRA         = None
        self.valid     = None
        self.seeddist  = 0
        self.seednum   = -1
        self.seedtxt   = None
        self.family    = None
        self.source    = None
        self.threshold = None
        self._bestseqs = None
        self.bgscale   = 1
        self.best_pvalue = None
        self.best_factor = None
        self.gamma     = None
        self.nbound    = 0
        self.matchids  = []
        self.overlap   = None
        self.cumP      = []
        self.numbound      = 0
        self.nummotif      = 0
        self.numboundmotif = 0
        if backgroundD:
            self.background = backgroundD
        else:
            self.background = {'A': 0.31, 'C': .19, 'G': .19, 'T': .31} #Yeast Default

        if type(list_of_seqs_or_text) == type(''):
            self.seqs      = []
            text           = list_of_seqs_or_text
            self.compute_from_text(text)
        else:
            self.seqs      = list_of_seqs_or_text
        if self.seqs:
            self._parse_seqs(list_of_seqs_or_text)
            self._compute_ll()
            self._compute_oneletter()
            #self._compute_threshold(2.0)

    def __repr__(self):
        return "%s (%d)"%(self.oneletter, self.nseqs)
    def summary(self):
        """
        m.summary() -- Return a text string one-line summary of motif and its metrics
        """
        m = self
        txt = "%-34s (Bits: %5.2f  MAP: %7.2f   D: %5.3f  %3d)  E: %7.3f"%(
            m, m.totalbits, m.MAP, m.seeddist, m.seednum, nlog10(m.pvalue))
        if m.binomial!=None:  txt = txt + '  Bi: %6.2f'%(nlog10(m.binomial))
        if m.church != None:  txt = txt + '  ch: %6.2f'%(nlog10(m.church))
        if m.frac   != None:  txt = txt + '  f: %5.3f'%(m.frac)
        if m.E_site != None:  txt = txt + '  Es: %6.2f'%(nlog10(m.E_site))
        if m.E_seq  != None:  txt = txt + '  Eq: %6.2f'%(nlog10(m.E_seq))
        if m.MNCP   != None:  txt = txt + '  mn: %6.2f'%(m.MNCP)
        if m.ROC_auc!= None:  txt = txt + '  Ra: %6.4f'%(m.ROC_auc)
        if m.E_chi2 != None:
            if m.E_chi2 == 0: m.E_chi2=1e-20
            txt = txt + ' x2: %5.2f'%(nlog10(m.E_chi2))
        if m.CRA    != None:  txt = txt + '  cR: %6.4f'%(m.CRA)
        if m.Cfrac  != None:  txt = txt + '  Cf: %5.3f'%(m.Cfrac)
        if m.realpvalue != None: txt = txt + '  P: %6.4e'%(m.realpvalue)
        if m.kellis != None:  txt = txt +  '  k: %6.2f'%(m.kellis)
        if m.numbound      :  txt = txt +  '  b: %3d'%(m.numbound)
        if m.nummotif      :  txt = txt +  '  nG: %3d'%(m.nummotif)
        if m.numboundmotif :  txt = txt +  '  bn: %3d'%(m.numboundmotif)

        return txt

    def minimal_raw_seqs(self):
        ''' m.minimal_raw_seqs() -- Return minimal list of seqs that represent consensus '''
        seqs = [[], []]
        for letter in self.oneletter:
            if one2two.has_key(letter):
                seqs[0].append(one2two[letter][0])
                seqs[1].append(one2two[letter][1])
            else:
                seqs[0].append(letter)
                seqs[1].append(letter)
        if ''.join(seqs[0]) == ''.join(seqs[1]):
            return( [''.join(seqs[0])] )
        else:
            return( [''.join(seqs[0]), ''.join(seqs[0])] )
    def _compute_oneletter(self):
        """
        m._compute_oneletter() -- [utility] Set the oneletter member variable
        """
        letters = []
        for i in range(self.width):
            downcase = None
            if self.bits[i] < 0.25:
                letters.append('.')
                continue
            if self.bits[i] < 1.0: downcase = 'True'
            tups = [(self.ll[i][x],x) for x in ACGT if self.ll[i][x] > 0.0]
            if not tups:  #Kludge if all values are negative (can this really happen?)
                tups = [(self.ll[i][x],x) for x in ACGT]
                tups.sort()
                tups.reverse()
                tups = [tups[0]]
                downcase = 'True'
            tups.sort()      #Rank by LL
            tups.reverse()
            bases = [x[1] for x in tups[0:2]]
            bases.sort()
            if len(bases) == 2: L = two2one[''.join(bases)]
            else:               L = bases[0]
            if downcase: L = L.lower()
            letters.append(L)
        self.oneletter = ''.join(letters)
    def _parse_seqs(self, LOS):
        """
        m._parse_seqs(LOS) -- [utility] Build a matrix of counts from a list of sequences
        """
        self.nseqs = len(LOS)
        self.width = len(LOS[0])
        for i in range(self.width):
            Dc = {'A': 0, 'C': 0, 'T': 0, 'G': 0, 'N': 0}
            for seq in LOS:
                key = seq[i]
                Dc[key] = Dc[key] + 1
            del(Dc['N'])
            self.counts.append(Dc)

    def _compute_ll(self):
        """
        m._compute_ll() -- [utility] Compute the log-likelihood matrix from the count matrix
        """
        self.fracs = []
        self.logP  = []
        self.ll    = []
        for i in range(self.width):
            Dll  = {'A': 0, 'C': 0, 'T': 0, 'G': 0}
            Df   = {'A': 0, 'C': 0, 'T': 0, 'G': 0}
            DlogP= {'A': 0, 'C': 0, 'T': 0, 'G': 0}
            for key in self.counts[i].keys():
                #print i,key,self.counts[i][key],self.nseqs
                Pij = self.counts[i][key] / float(self.nseqs)
                Df [key] = Pij
                Dll[key] = (math.log( (self.counts[i][key] + self.bgscale*self.background[key] ) / 
                                      ((self.nseqs + self.bgscale) * self.background[key])     ) /
                            math.log(2))
                if Pij > 0:
                    DlogP[key]  = math.log(Pij)/math.log(2)
                else:
                    DlogP[key]  = -100  #Near zero
            self.fracs.append(Df)
            self.logP.append (DlogP)
            self.ll.append   (Dll)
        self.P = self.fracs
        self._compute_bits()
        self._compute_ambig_ll()
        self._maxscore()


    def compute_from_ll(self,ll):
        """
        m.compute_from_ll(ll) -- Build motif from an inputed log-likelihood matrix

        (This function reverse-calculates the probability matrix and background frequencies
        that were used to construct the log-likelihood matrix)
        """
        self.ll    = ll
        self.width = len(ll)
        self._compute_bg_from_ll()
        self._compute_logP_from_ll()
        self._compute_ambig_ll()
        self._compute_bits()
        self._compute_oneletter()
        self._maxscore()

    def _computeP(self):
        """
        m._computeP() -- [utility] Compute the probability matrix (from the internal log-probability matrix)
        """
        P = []
        for i in range(self.width):
            #print i,
            _p = {}
            for L in ACGT: _p[L] = math.pow(2.0,self.logP[i][L])
            P.append(_p)
        #print
        self.P = P

    def _compute_bits(self):
        """
        m._compute_bits() -- [utility] Set m.totbits to the number of bits and m.bits to a list of bits at each position
        """
        bits = []
        totbits = 0
        bgbits  = 0
        bg      = self.background
        UNCERT  = lambda x: x*math.log(x)/math.log(2.0)
        for letter in ACGT:
            bgbits = bgbits + UNCERT(bg[letter])
        for i in range(self.width):
            tot = 0
            for letter in ACGT:
                Pij = pow(2.0, self.logP[i][letter])
                tot = tot + UNCERT(Pij)
                #bit = Pij * self.ll[i][letter]
                #if bit > 0:
                #    tot = tot + bit
            #print tot, bgbits, tot-bgbits
            bits.append(max(0,tot-bgbits))
            totbits = totbits + max(0,tot-bgbits)
        self.bits = bits
        self.totalbits = totbits

        
    def denoise(self,bitthresh=0.5):
        """
        m.denoise(bitthresh=0.5) -- Set low-information positions (below bitthresh) to Ns
        """
        for i in range(self.width):
            tot = 0
            for letter in ACGT:
                if self.logP:
                    Pij = pow(2.0, self.logP[i][letter])
                else:
                    Pij = pow(2.0, self.ll[i][letter]) * self.background[letter]
                if Pij > 0.01:
                    bit = Pij * self.ll[i][letter]
                    tot = tot + bit
            if tot < bitthresh:  #Zero Column
                for letter in ACGT:
                    self.ll[i][letter] = 0.0
        self.compute_from_ll(self.ll)

    def giflogo(self,id,title=None,scale=0.8,info_str=''):
        """
        m.giflogo(id,title=None,scale=0.8) -- (Requires seqlogo package) Make a gif sequence logo
        """
        return giflogo(self,id,title,scale)

    def printlogo(self,norm=2.3, height=10.0):
        """
        m.printlogo(,norm=2.3, height=10.0) -- Print a text-rendering of the Motif Logo

        norm   -- maximum number of bits to show
        height -- number of lines of text to use to render logo
        """
        self._print_bits(norm,height)
    def print_textlogo(self,norm=2.3, height=8.0):
        """
        m.print_textlogo(,norm=2.3, height=8.0) -- Print a text-rendering of the Motif Logo

        norm   -- maximum number of bits to show
        height -- number of lines of text to use to render logo
        """
        self._print_bits(norm,height)
    def _print_bits(self,norm=2.3, height=8.0):
        """
        m._print_bits(,norm=2.3, height=8.0) -- Print a text-rendering of the Motif Logo

        norm   -- maximum number of bits to show
        height -- number of lines of text to use to render logo
        """
        bits   = []
        tots   = []
        str    = []
        for i in range(self.width):
            D = {}
            tot = 0
            for letter in ['A', 'C', 'T', 'G']:
                if self.logP:
                    Pij = pow(2.0, self.logP[i][letter])
                else:
                    Pij = pow(2.0, self.ll[i][letter]) * self.background[letter]
                if Pij > 0.01:
                    '''Old'''
                    D[letter] = Pij * self.ll[i][letter]
                    #'''new'''
                    #Q = self.background[letter]
                    #D[letter] = ( Pij * math.log(Pij) - Pij * math.log(Q) ) / math.log(2.0)
                    '''for both old and new'''
                    tot = tot + D[letter]
            bits.append(D)
            tots.append(tot)
        for i in range(self.width):
            s = []
            _l = bits[i].keys()
            _l.sort(lambda x,y,D=bits[i]: cmp(D[y],D[x]))
            for key in _l:
                for j in range(int(bits[i][key] / norm * height)):
                    s.append(key)
            str.append(''.join(s))
        fmt = '%%%ds'%height
        print '#  %s'%('-'*self.width)
        for h in range(int(height)):
            sys.stdout.write("#  ")
            for i in range(self.width):
                sys.stdout.write((fmt%str[i])[h])
            if h == 0:
                sys.stdout.write(' -- %4.2f bits\n'%norm)
            elif h == height-1:
                sys.stdout.write(' -- %4.2f bits\n'%(norm/height))
            else:
                sys.stdout.write('\n')
        print '#  %s'%('-'*self.width)
        print '#  %s'%self.oneletter

    def _compute_ambig_ll(self):
        """
        m._compute_ambig_ll() -- Extend log-likelihood matrix to include ambiguity codes
                                 e.g.  What the score of a 'S'?  Here we use the max of C and G.
        """
        for Dll in self.ll:
            for L in one2two.keys():
                Dll[L] = max(Dll[one2two[L][0]],  Dll[one2two[L][1]] )
            Dll['N'] = 0.0
            Dll['B'] = 0.0

    def compute_from_nmer(self,nmer,beta=0.001):  #For reverse compatibility
        """
        m.compute_from_nmer(nmer,beta=0.001):  See compute_from_text.  Here for reverse compatibility
        """
        self.compute_from_text(nmer,beta)

    def compute_from_text(self,text,beta=0.001):
        """
        m.compute_from_text(,text,beta=0.001)  -- Compute a matrix values from a text string of ambiguity codes.
                                                  Use Motif_from_text utility instead to build motifs on the fly.
        """
        prevlett = {'B':'A', 'D':'C', 'V':'T', 'H':'G'}
        countmat = []
        text = re.sub('[\.\-]','N',text.upper())
        for i in range(len(text)):
            D = {'A': 0, 'C': 0, 'T':0, 'G':0}
            letter = text[i]
            if letter in ['B', 'D', 'V', 'H']:  #B == no "A", etc...
                _omit = prevlett[letter]
                for L in ACGT:
                    if L != _omit: D[L] = 0.3333
            elif one2two.has_key(letter):  #Covers WSMYRK
                for L in list(one2two[letter]):
                    D[L] = 0.5
            elif letter == 'N':
                for L in D.keys():
                    D[L] = self.background[L]
            elif letter == '@':
                for L in D.keys():
                    D[L] = self.background[L]-(0.0001)
                D['A'] = D['A'] + 0.0004
            else:
                D[letter] = 1.0
            countmat.append(D)
        self.compute_from_counts(countmat,beta)

    def new_bg(self,bg):
        """
        m.new_bg(,bg)  -- Change the ACGT background frequencies to those in the supplied dictionary.
                          Recompute log-likelihood, etc. with new background.
        """
        counts = []
        for pos in self.logP:
            D = {}
            for L,lp in pos.items():
                D[L] = math.pow(2.0,lp)
            counts.append(D)
        self.background = bg
        self.compute_from_counts(counts,0)
        
    def addpseudocounts(self,beta=0):
        """
        m.addpseudocounts(,beta=0) -- Add pseudocounts uniformly across the matrix
        """
        self.compute_from_counts(self.counts,beta)
    
    def compute_from_counts(self,countmat,beta=0):
        """
        m.compute_from_counts(,countmat,beta=0) -- Utility function to build a motif object from a matrix of letter counts.
        """
        self.counts  = countmat
        self.width   = len(countmat)
        self.bgscale = 0

        maxcount = 0
        #Determine Biggest column
        for col in countmat:
            tot = 0
            for v in col.values():
                tot = tot + v
            if tot > maxcount: maxcount = tot

        #Pad counts of remaining columns
        for col in countmat:
            tot = 0
            for c in col.values():
                tot = tot + c
            pad = maxcount - tot
            for L in col.keys():
                col[L] = col[L] + pad * self.background[L]
                
        self.nseqs = maxcount
        nseqs      = maxcount
        
        #Add pseudocounts        
        if beta > 0:  
            multfactor = {}
            bgprob = self.background
            pcounts= {}
            for L in bgprob.keys():
                pcounts[L] = beta*bgprob[L]*nseqs 
            for i in range(self.width):
                for L in countmat[i].keys():
                    _t = (countmat[i][L] + pcounts[L]) #Add pseudo
                    _t = _t / (1.0 + beta)    #Renomalize
                    countmat[i][L] = _t

        #Build Motif
        self.counts = countmat
        self._compute_ll()
        self._compute_oneletter()
        self._maxscore()


    def _compute_bg_from_ll(self):
        """
        m._compute_bg_from_ll()

        Compute background model from log-likelihood matrix
        by noting that:   pA  + pT  + pC  + pG  = 1
                  and     bgA + bgT + bgC + bgG = 1
                  and     bgA = bgT,   bgC = bgG
                  and so  bgA = 0.5 - bgC
                  and     pA  = lA * bgA,  etc for T, C, G
                  so...
                         (lA + lT)bgA + (lC + lG)bgC          =  1
                         (lA + lT)bgA + (lC + lG)(0.5 - bgA)  =  1
                         (lA + lT - lC - lG)bgA +(lC +lG)*0.5 =  1
                          bgA                                 =  {1 - 0.5(lC + lG)} / (lA + lT - lC - lG)
        + Gain accuracy by taking average of bgA over all positions of PSSM
        """
        
        pow = math.pow
        bgATtot = 0
        nocount = 0
        near0   = lambda x:(-0.01 < x and x < 0.01)
        for i in range(self.width):
            _D = self.ll[i]
            ATtot = pow(2,_D['A']) + pow(2,_D['T'])
            GCtot = pow(2,_D['C']) + pow(2,_D['G'])
            if near0(_D['A']) and near0(_D['T']) and near0(_D['G']) and near0(_D['C']):
                nocount = nocount + 1
                continue
            if near0(ATtot-GCtot):     #Kludge to deal with indeterminate case
                nocount = nocount + 1
                continue
            bgAT   = (1.0 - 0.5*GCtot)/(ATtot - GCtot)
            if (bgAT < 0.1) or (bgAT > 1.1):
                nocount = nocount + 1
                continue
            bgATtot = bgATtot + bgAT
        if nocount == self.width:  #Kludge to deal with different indeterminate case
            self.background = {'A':0.25, 'C':0.25, 'G':0.25, 'T':0.25}
            return
        bgAT = bgATtot / (self.width - nocount)
        bgGC = 0.5 - bgAT
        self.background = {'A':bgAT, 'C':bgGC, 'G':bgGC, 'T':bgAT}            
        
    def _compute_logP_from_ll(self):
        """
        m._compute_logP_from_ll() -- Compute self's logP matrix from the self.ll (log-likelihood)
        """
        log = math.log
        logP = []
        for i in range(self.width):
            D = {}
            for L in ACGT:
                ''' if   ll = log(p/b) then
                       2^ll = p/b
                  and    ll = log(p) - log(b)
                  so log(p) = ll + log(b)'''
                #Pij = pow(2.0, self.ll[i][letter]) * self.background[letter]
                D[L] = self.ll[i][L] + log(self.background[L])/log(2.)
            logP.append(D)
        self.logP = logP

    def _print_ll(self):
        """
        m._print_ll() -- Print log-likelihood (scoring) matrix
        """
        print "#  ",
        for i in range(self.width):
            print "  %4d   "%i,
        print
        for L in ['A', 'C', 'T', 'G']:
            print "#%s "%L,
            for i in range(self.width):
                print  "%8.3f "%self.ll[i][L],
            print
    def _print_p(self):
        """
        m._print_p() -- Print probability (frequency) matrix
        """
        print "#  ",
        for i in range(self.width):
            print "  %4d   "%i,
        print
        for L in ['A', 'C', 'T', 'G']:
            print "#%s "%L,
            for i in range(self.width):
                print  "%8.3f "%math.pow(2,self.logP[i][L]),
            print
    def _print_counts(self):
        """
        m._print_counts() -- Print count matrix 
        """
        print "#  ",
        for i in range(self.width):
            print "  %4d   "%i,
        print
        for L in ['A', 'C', 'T', 'G']:
            print "#%s "%L,
            for i in range(self.width):
                print  "%8.3f "%self.counts[i][L],
            print
        
    def _maxscore(self):
        """
        m._maxscore() -- Sets self.maxscore and self.minscore 
        """
        total = 0
        lowtot= 0
        for lli in self.ll:
            total = total + max(lli.values())
            lowtot= lowtot+ min(lli.values())
        self.maxscore = total
        self.minscore = lowtot

    def _compute_threshold(self,z=2.0):
        """
        m._compute_threshold(z=2.0) -- For Motif objects assembled from a set of sequence,
                                       compute a self.threshold with a z-score based on the distribution
                                       of scores in among the original input sequences.
        """
        scoretally = []
        for seq in self.seqs:
            matches,endpoints,scores = self.scan(seq,-100)
            scoretally.append(scores[0])
        ave,std = avestd(scoretally)
        self.threshold = ave - z *std
        #print '#%s: threshold %5.2f = %5.2f - %4.1f * %5.2f'%(
        #    self, self.threshold, ave, z, std)

    def bestscanseq(self,seq):
        """
        m.bestscanseq(seq) -- Return score,sequence of the best match to the motif in the supplied sequence
        """
        matches,endpoints,scores = self.scan(seq,-100)
        t = zip(scores,matches)
        t.sort()
        bestseq   = t[-1][1]
        bestscore = t[-1][0]
        return bestscore, bestseq
    
    def bestscore(self,seq):
        """
        m.bestscore(seq) -- Return the score of the best match to the motif in the supplied sequence
        """
        return m.bestscan(seq)
    def bestscan(self,seq):
        """
        m.bestscan(seq) -- Return the score of the best match to the motif in the supplied sequence
        """
        matches,endpoints,scores = self.scan(seq,-100)
        if not scores: return -100
        scores.sort()
        best = scores[-1]
        return best

    def matchstartorient(self,seq, factor=0.7):
        """
        m.matchstartorient(,seq, factor=0.7) -- Returns list of (start,orientation) coordinate pairs of
                                                matches to the motif in the supplied sequence.  Factor
                                                is multiplied by m.maxscore to get a match threshold.
        """
        ans = []
        txts,endpoints,scores = self.scan(seq,factor=factor)
        for txt, startstop in zip(txts,endpoints):
            start, stop = startstop
            rctxt  = revcomplement(txt)
            orient = (self.bestscore(txt,1) >= self.bestscore(rctxt,1))
            ans.append((start,orient))
        return ans

    def scan(self, seq, threshold = '', factor=0.7):
        """
        m.scan(seq, threshold = '', factor=0.7) -- Scan the sequence.  Returns three lists:
                                                   matching sequences, endpoints, and scores.  The value of 
                                                   'factor' is multiplied by m.maxscore to get a match threshold
                                                   if none is supplied
        """
        if len(seq) < self.width:
            return(self._scan_smaller(seq,threshold))
        else:
            return(self._scan(seq,threshold,factor=factor))

    def scansum(self,seq,threshold = -1000):
        """
        m.scansum(seq,threshold = -1000) -- Sum of scores over every window in the sequence.  Returns
                                            total, number of matches above threshold, average score, sum of exp(score)
        """
        ll = self.ll
        sum = 0
        width        = self.width
        width_r      = range(width)
        width_rcr    = range(width-1,-1,-1)
        width_ranges = zip(width_r,width_rcr)
        seqcomp      = seq.translate(revcompTBL)

        total = 0
        hits  = 0
        etotal= 0
        for offset in range(len(seq)-width+1):
            total_f = 0
            total_r = 0
            for i,ir in width_ranges:
                pos = offset+i
                total_f = total_f + ll[i][    seq[pos]]
                total_r = total_r + ll[i][seqcomp[pos]]
            total_max = max(total_f,total_r)
            if total_max >= threshold:
                total = total + total_max
                etotal = etotal + math.exp(total_max)
                hits  = hits + 1
            if not hits:
                ave = 0
            else:
                ave = float(total)/float(hits)
        return(total,hits,ave,math.log(etotal))
    def score(self, seq, fwd='Y'):
        """
        m.score(seq, fwd='Y') -- Returns the score of the first w-bases of the sequence, where w is the motif width.
        """
        matches, endpoints, scores = self._scan(seq,threshold=-100000,forw_only=fwd)
        return scores[0]
    def bestscore(self,seq, fwd=''):
        """
        m.bestscore(seq, fwd='') -- Returns the score of the best matching subsequence in seq.
        """
        matches, endpoints, scores = self._scan(seq,threshold=-100000,forw_only=fwd)
        if scores: return max(scores)
        else:      return -1000

    def _scan(self, seq,threshold='',forw_only='',factor=0.7):
        """
        m._scan(seq,threshold='',forw_only='',factor=0.7) -- Internal tility function for performing sequence scans
        """
        ll = self.ll #Shortcut for Log-likelihood matrix
        if not threshold: threshold = factor * self.maxscore
        
        #print '%5.3f'%(threshold/self.maxscore)
        matches       = []
        endpoints     = []
        scores        = []
        width         = self.width
        width_r       = range(width)
        width_rcr     = range(width-1,-1,-1)
        width_ranges  = zip(width_r,width_rcr)

        
        oseq = seq
        seq  = seq.upper()
        seqcomp = seq.translate(revcompTBL)
        for offset in range(len(seq)-self.width+1):    #Check if +/-1 needed
            total_f = 0
            total_r = 0
            for i,ir in width_ranges:
                pos = offset+i
                total_f = total_f + ll[i ][    seq[pos]]
                total_r = total_r + ll[ir][seqcomp[pos]]

            if 0 and total_f > 1:
                for i,ir in width_ranges:
                    print seq[offset+i],'%6.3f'%ll[i ][        seq[offset+i] ],'   ',
                print '= %7.3f'%total_f
                
            if 0:
                print "\t\t%s vs %s: F=%6.2f R=%6.2f %6.2f %4.2f"%(seq[offset:offset+self.width],
                                                                   self.oneletter,total_f,total_r,
                                                                   self.maxscore,
                                                                   max([total_f,total_r])/self.maxscore)
            if total_f > threshold and ((total_f > total_r) or forw_only):
                endpoints.append( (offset,offset+self.width-1) )
                scores.append(total_f)
                matches.append(oseq[offset:offset+self.width])
            elif total_r > threshold:
                endpoints.append( (offset,offset+self.width-1) )
                scores.append(total_r)
                matches.append(oseq[offset:offset+self.width])
        return(matches,endpoints,scores)
    def _scan_smaller(self, seq, threshold=''):
        """
        m._scan_smaller(seq, threshold='') -- Internal utility function for performing sequence scans

        The sequence is smaller than the PSSM.  Are there
        good matches to regions of the PSSM?
        """
        ll = self.ll #Shortcut for Log-likelihood matrix
        matches   = []
        endpoints = []
        scores    = []
        w         = self.width
        oseq      = seq
        seq       = seq.upper()
        for offset in range(self.width-len(seq)+1):    #Check if +/-1 needed
            maximum = 0
            for i in range(len(seq)):
                maximum = maximum + max(ll[i+offset].values())
            if not threshold: threshold = 0.8 * maximum
            total_f = 0
            total_r = 0
            for i in range(len(seq)):
                total_f = total_f + ll[i+offset      ][        seq[i] ]
                total_r = total_r + ll[w-(i+offset)-1][revcomp[seq[i]]]
            if 0:
                print "\t\t%s vs %s: F=%6.2f R=%6.2f %6.2f %4.2f"%(oseq, self.oneletter[offset:offset+len(seq)],
                                                                   total_f, total_r,  maximum,
                                                                   max([total_f,total_r])/self.maxscore)
            if total_f > threshold and total_f > total_r:
                endpoints.append( (offset,offset+self.width-1) )
                scores.append(total_f)
                matches.append(oseq[offset:offset+self.width])
            elif total_r > threshold:
                endpoints.append( (offset,offset+self.width-1) )
                scores.append(total_r)
                matches.append(oseq[offset:offset+self.width])
        return(matches,endpoints,scores)                

    def mask_seq(self,seq):
        """
        m.mask_seq(seq) -- Return a copy of input sequence in which any regions matching m are replaced with strings of N's
        """
        masked = ''
        matches, endpoints, scores = self.scan(seq)
        cursor = 0
        for start, stop in endpoints:
            masked = masked + seq[cursor:start] + 'N'*self.width
            cursor = stop+1
        masked = masked + seq[cursor:]
        return masked

    def masked_neighborhoods(self,seq,flanksize):
        """
        m.masked_neighborhoods(seq,flanksize) -- Chop up the input sequence into regions surrounding matches to m.  Replace the
                                                 subsequences that match the motif with N's.
        """
        ns = self.seq_neighborhoods(seq,flanksize)
        return [self.mask_seq(n) for n in ns]

    def seq_neighborhoods(self,seq,flanksize):
        """
        m.seq_neighborhoods(seq,flanksize) -- Chop up the input sequence into regions surrounding matches to the motif.
        """
        subseqs = []
        matches, endpoints, scores = self.scan(seq)
        laststart, laststop = -1, -1
        for start, stop in endpoints:
            curstart, curstop = max(0,start-flanksize), min(stop+flanksize,len(seq))
            if curstart > laststop:
                if laststop != -1:
                    subseqs.append(seq[laststart:laststop])
                laststart, laststop = curstart, curstop
            else:
                laststop = curstop
        if endpoints: subseqs.append(seq[laststart:laststop])
        return subseqs

    def __sub__(self,other):
        """
        m.__sub__(other)  --- Overloads the '-' operator to compute the Euclidean distance between probability matrices
                              motifs of equal width.  Consult TAMO.Clustering.MotifCompare for metrics to compare motifs
                              of different widths
        """
        if type(other) != type(self):
            print "computing distance of unlike pssms (types %s, %s)"%(
                type(other),type(self))
            print 'First: %s'%other
            print 'Self:  %s'%self
            sys.exit(1)
        if other.width != self.width:
            print "computing distance of unlike pssms (width %d != %d)"%(
                other.width,self.width)
            sys.exit(1)
        D = 0
        FABS = math.fabs
        POW  = math.pow
        for L in self.logP[0].keys():
            for i in range(self.width):
                D = D + POW( POW(2,self.logP[i][L]) - POW(2,other.logP[i][L]), 2 )
                #D = D + FABS( POW(2,self.logP[i][L]) - POW(2,other.logP[i][L]))
                #D = D + FABS(self.logP[i][L] - other.logP[i][L])
        return(math.sqrt(D))

    def maskdiff(self,other):
        """
        m.maskdiff(other) -- A different kind of motif comparison metric.  See THEME paper for details
        """
        return maskdiff(self,other)

    def maxdiff(self):
        """
        m.maxdiff()   -- Compute maximum possible Euclidean distance to another motif.  (For normalizing?)
        """
        POW  = math.pow
        D = 0
        for i in range(self.width):
            _min = 100
            _max = -100
            for L in ACGT:
                val = POW(2,self.logP[i][L])
                if   val > _max:
                    _max  = val
                    _maxL = L
                elif val < _min:
                    _min  = val
                    _minL = L
            for L in ACGT:
                if L == _minL:
                    delta = 1-POW(2,self.logP[i][L])           #1-val
                    D = D + delta*delta
                else:
                    D = D + POW( POW(2,self.logP[i][L]), 2)    #0-val
        return(math.sqrt(D))
                
    def revcomp(self):
        """
        m.revcomp() -- Return reverse complement of motif
        """
        return revcompmotif(self)
    def trimmed(self,thresh=0.1):
        """
        m.trimmed(,thresh=0.1)  -- Return motif with low-information flanks removed.  'thresh' is in bits.
        """
        for start in range(0,self.width-1):
            if self.bits[start]>=thresh: break
        for stop  in range(self.width,1,-1):
            if self.bits[stop-1]>=thresh: break
        m = self[start,stop]
        return m
    def bestseqs(self,thresh=None):
        """
        m.bestseqs(,thresh=None)  -- Return all k-mers that match motif with a score >= thresh
        """
        if not thresh:
            if self._bestseqs:
                return self._bestseqs
        if not thresh: thresh = 0.8 * self.maxscore
        self._bestseqs = bestseqs(self,thresh)
        return self._bestseqs
    def emit(self,prob_min=0.0,prob_max=1.0):
        """
        m.emit(,prob_min=0.0,prob_max=1.0) -- Consider motif as a generative model, and have it emit a sequence
        """
        if not self.cumP:
            for logcol in self.logP:
                tups = []
                for L in ACGT:
                    p = math.pow(2,logcol[L])
                    tups.append((p,L))
                tups.sort()
                cumu = []
                tot  = 0
                for p,L in tups:
                    tot = tot + p
                    cumu.append((tot,L))
                self.cumP.append(cumu)
        s = []
        #u = random()+0.01 #Can make higher for more consistent motifs
        u = (prob_max-prob_min)*random() + prob_min
        for cumu in self.cumP:
            #u = random()+0.01 #Can make higher for more consistent motifs
            last = 0
            for p,L in cumu:
                if last < u and u <= p:
                    letter = L
                    break
                else: last = p
#           print L,'%8.4f'%u,cumu
            s.append(L)
        #print ''.join(s)
        return ''.join(s)
            
                
    def random_kmer(self):
        """
        m.random_kmer() -- Generate one of the many k-mers that matches the motif.  See m.emit() for a more probabilistic generator
        
        """
        if not self._bestseqs: self._bestseqs = self.bestseqs()
        seqs   = self._bestseqs
        pos = int(random() * len(seqs))
        print 'Random: ',self.oneletter,seqs[pos][1]
        return(seqs[pos][1])
    def __getitem__(self,tup):
        """
        m.__getitem__(tup) -- Overload m[a,b] to submotif.  Less pythonish than [:], but more reliable
        """
        if len(tup) != 2:
            print "Motif[i,j] requires two arguments, not ",tup
        else:
            beg, end = tup[0], tup[1]
            return(submotif(self,beg,end))
    def __getslice__(self,beg,end):
        """
        m.__getslice__(,beg,end) -- Overload m[a:b] to submotif.
        """
        if beg >= end:
            #Probably python converted negative idx.  Undo
            beg = beg - self.width
        return(submotif(self,beg,end))
    def __add__(self,other):
        """
        m.__add__(other) -- Overload  '+' for concatenating motifs
        """
        return merge(self,other,0)
    def __len__(self):
        """
        m.__len__()  -- Overload len(m) to return width
        """
        return(self.width)
    def shuffledP(self):
        """
        m.shuffledP() -- Generate motif in which probability matrix has been shuffled.
        """
        return shuffledP(self)
    def copy(self):
        """
        m.copy() -- Return a 'deep' copy of the motif
        """
        a = Motif()
        a.__dict__ = self.__dict__.copy()
        return a
    def random_diff_avestd(self,iters=5000):
        """
        m.random_diff_avestd(iters=5000) -- See modules' random_diff_avestd
        """
        return(random_diff_avestd(self,iters))
    def bogus_kmers(self,count=200):
        """
        m.bogus_kmers(count=200) --  Generate a faked multiple sequence alignment that will reproduce
                                     the probability matrix.
        """

        POW  = math.pow
        #Build p-value inspired matrix
        #Make totals cummulative:
        # A: 0.1 C: 0.4 T:0.2 G:0.3
        #                            ->  A:0.0 C:0.1 T:0.5 G:0.7  0.0
        
        #Take bg into account:
        # We want to pick P' for each letter such that:
        #     P'/0.25  = P/Q
        # so  P'       = 0.25*P/Q
        
        m = []
        for i in range(self.width):
            _col = []
            tot   = 0.0
            for L in ACGT:
                _col.append( tot )
                tot = tot + POW(2,self.logP[i][L]) * 0.25 / self.background[L]
            _col.append(tot)
            #Renormalize
            for idx in range(len(_col)):
                _col[idx] = _col[idx] / _col[-1]
            m.append(_col)

        for p in range(0): #Was 5
            for i in range(len(m)):
                print '%6.4f  '%m[i][p],
            print

        seqs=[]
        for seqnum in range(count+1):
            f = float(seqnum)/(count+1)
            s = []
            for i in range(self.width):
                for j in range(4):
                    if (m[i][j] <= f and f < m[i][j+1]):
                        s.append(ACGT[j])
                        break
            seqs.append(''.join(s))

        del(seqs[0])
        #for i in range(count):
        #    print ">%3d\n%s"%(i,seqs[i])

        return(seqs)


def seqs2fasta(seqs,fasta_file = ''):
    if not fasta_file:
        fasta_file = tempfile.mktemp()
    FH = open(fasta_file,'w')
    for i in range(len(seqs)):
        FH.write(">%d\n%s\n"%(i,seqs[i]))
    FH.close()
    return(fasta_file)

'''

END FUNCTION FROM TAMO
'''

#CUSTOM FUNCTIONS ADDED to the TAMO LIBRARY FILE
import subprocess as sb

def pwm_from_meme_file(meme_filename,motif_id):
    found=False
    with open(meme_filename) as meme_file:
        #find the first line 

        line = meme_file.readline()
        while line:
            if 'MOTIF' in line and motif_id in line:
                found=True
                #print line
                break
            line = meme_file.readline()

        if not found:
            return None
        else:
            found=False
            #look for the letter-probability matrix
            line = meme_file.readline()
            while line:
                if 'letter-probability matrix' in line:
                    found=True
                    #print line

                    motif_length=int(re.search(r'w= *(?P<length>\d+)',line).groupdict()['length'])
                    #print motif_length
                    break
                line = meme_file.readline()
                    

            pwm=[]
            for i in range(motif_length):
                line=meme_file.readline()
                probabilities=map(float,line.split())
                #print probabilities
                pwm.append({'A':probabilities[0],'C':probabilities[1],'G':probabilities[2],'T':probabilities[3]})
            return pwm   


def generate_weblogo(motif_id,meme_filename,output_filename,file_format='png',title=None):

    if title==None: title = output_filename

    output_filename = output_filename + '.' + file_format

    pwm=pwm_from_meme_file(meme_filename,motif_id)
    meme_motif=Motif_from_counts(pwm)

    kmers   = meme_motif.bogus_kmers(100)
    #width   = float(len(kmers[0]) ) *scale
    #height  = float(4)
    #width, height = width*scale, height*scale
    tmp     = tempfile.mktemp() + '.fsa'
    seqs2fasta(kmers,tmp)

    tmp= "/home/rick/cap.fa"
    cmd = 'weblogo -F %s  -A dna  -f %s  -o %s  -t "%s"   --errorbars NO  --fineprint "" '%(file_format, tmp, output_filename, title)
    #print cmd 
    sb.call(cmd,shell=True)

