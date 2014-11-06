
MemeMenu.blurbs = {
  "meme": "<dfn>MEME</dfn> discovers novel, <em>ungapped</em> motifs (recurring, fixed-length patterns) in your <em>nucleotide or protein</em> sequences<nav> (<a href=\"../doc/examples/meme_example_output_files/meme.html\">sample output</a>)</nav>. <dfn>MEME</dfn> splits variable-length patterns into two or more separate motifs.",
  "dreme": "<dfn>DREME</dfn> discovers <em>short</em>, <em>ungapped</em> motifs (recurring, fixed-length patterns) that are <em>relatively</em> enriched in your <em>nucleotide</em> sequences compared with shuffled sequences or your control sequences<nav> (<a href=\"../doc/examples/dreme_example_output_files/dreme.html\">sample output</a>). See this <a href=\"../doc/dreme-tutorial.html\">Tutorial</a> for more information</nav>.",
  "memechip": "<dfn>MEME-ChIP</dfn> performs <em>comprehensive motif analysis</em> (including motif discovery) on LARGE (50MB maximum) sets of <em>nucleotide</em> sequences such as those identified by ChIP-seq or CLIP-seq experiments<nav> (<a href=\"../doc/examples/memechip_example_output_files/index.html\">sample output</a>)</nav>.",
  "glam2": "<dfn>GLAM2</dfn> discovers novel, <em>gapped</em> motifs (recurring, variable-length patterns) in your <em>DNA</em> or <em>protein</em> sequences<nav> (<a href=\"../doc/examples/glam2_example_output_files/glam2.html\">sample output</a>)</nav>.",
  "ame": "<dfn>AME</dfn> identifies <em>known</em> or <em>user-provided</em> motifs that are <em>relatively</em> enriched in your nucleotide sequences compared with shuffled sequences or your control sequences<nav> (<a href=\"../doc/examples/ame_example_output_files/ame.html\">sample output</a>)</nav>. <dfn>AME</dfn> treats motif occurrences the same, regardless of their locations within the sequences.", 
  "centrimo": "<dfn>CentriMo</dfn> identifies <em>known</em> or <em>user-provided</em> motifs that show a significant preference for particular <em>locations</em> in your nucleotide sequences<nav> (<a href=\"../doc/examples/centrimo_example_output_files/centrimo.html\">sample output</a>)</nav>. <dfn>CentriMo</dfn> can also show if the local enrichment is significant <em>relative</em> to control sequences.",
  "spamo": "<dfn>SpaMo</dfn> identifies significantly enriched <em>spacings</em> in a set of sequences between a <em>primary motif</em> and each motif in a set of <em>secondary motifs</em><nav> (<a href=\"../doc/examples/spamo_example_output_files/spamo.html\">sample output</a>)</nav>. Typically, the input sequences are centered on ChIP-seq peaks.",
  "gomo": "<dfn>GOMO</dfn> scans all <em>promoters</em> using <em>nucleotide motifs</em> you provide to determine if any motif is significantly associated with genes linked to one or more Genome Ontology (GO) terms<nav> (<a href=\"../doc/examples/gomo_example_output_files/gomo.html\">sample output</a>)</nav>. The significant GO terms can suggest the <em>biological roles</em> of the motifs.",
  "fimo": "<dfn>FIMO</dfn> scan a nucleotide or protein sequence database for <em>individual matches</em> to each of the motifs you provide<nav> (<a href=\"../doc/examples/fimo_example_output_files/fimo.html\">sample output</a>)</nav>.", 
  "mast": "<dfn>MAST</dfn> searches sequences for matches to a set of <em>nucleotide</em> or <em>protein</em> motifs, and sorts the sequences by the <em>best combined match</em> to all motifs<nav> (<a href=\"../doc/examples/mast_example_output_files/mast.html\">sample output</a>)</nav>.",
  "mcast": "<dfn>MCAST</dfn> searches sequences for <em>clusters of matches</em> to one or more <em>nucleotide</em> motifs<nav> (<a href=\"../doc/examples/mcast_example_output_files/mcast.html\">sample output</a>)</nav>.",
  "glam2scan": "<dfn>GLAM2Scan</dfn> searches sequences for matches to gapped <em>DNA</em> or <em>protein</em> GLAM2 motifs<nav> <a href=\"../doc/examples/glam2scan_example_output_files/glam2scan.html\">sample output</a>)</nav>.",
  "tomtom": "<dfn>Tomtom</dfn> compares one or more <em>DNA</em> motifs against a database of known motifs (e.g., JASPAR). <dfn>Tomtom</dfn> will rank the motifs in the database and produce an alignment for each significant match<nav> (<a href=\"../doc/examples/tomtom_example_output_files/tomtom.html\">sample output</a>)</nav>."
};

/*
 * Three types of URLS
 * 1) Relative URLs -
 *      Displayed all the time. These will have the path prefix appended. This
 *      type of URL is the default.
 * 2) Webserver URLs - 
 *      Only displayed when running on a webserver or if a webserver URL has
 *      been provided. When running on a webserver these should behave like
 *      relative URLs and have the path prefix appended, otherwise they should
 *      have the full server_url appended.
 * 3) Absolute URLs -
 *      Displayed all the time. These always go to the same site.
 *
 */
MemeMenu.data = {
  "header": {
    "title": "MEME Suite @VERSION@",
    "doc_url": "doc/overview.html",
    "web_url": "index.html"
  },
  "topics": [
    {
      "title": "Motif Discovery",
      "info": "De novo discovery of motifs in set(s) of sequences.",
      "server": true,
      "topics": [
        {
          "title": "MEME",
          "info": MemeMenu.blurbs["meme"],
          "url": "tools/meme",
          "server": true
        }, {
          "title": "DREME",
          "info": MemeMenu.blurbs["dreme"],
          "url": "tools/dreme",
          "server": true
        }, {
          "title": "MEME-ChIP",
          "info": MemeMenu.blurbs["memechip"],
          "url": "tools/meme-chip",
          "server": true
        }, {
          "title": "GLAM2",
          "info": MemeMenu.blurbs["glam2"],
          "url": "tools/glam2",
          "server": true
        }
      ]
    }, {
      "title": "Motif Enrichment",
      "info": "Analyze sequence set(s) for enrichment of known motifs.",
      "server": true,
      "topics": [
        {
          "title": "AME",
          "info": MemeMenu.blurbs["ame"],
          "url": "tools/ame",
          "server": true
        },
        {
          "title": "CentriMo",
          "info": MemeMenu.blurbs["centrimo"],
          "url": "tools/centrimo",
          "server": true
        }, {
          "title": "SpaMo",
          "info": MemeMenu.blurbs["spamo"],
          "url": "tools/spamo",
          "server": true
        }, {
          "title": "GOMo",
          "info": MemeMenu.blurbs["gomo"],
          "url": "tools/gomo",
          "server": true
        }
      ]
    }, {
      "title": "Motif Scanning",
      "info": "Find matches to motifs in sequences.",
      "server": true,
      "topics": [
        {
          "title": "FIMO",
          "info": MemeMenu.blurbs["fimo"],
          "url": "tools/fimo",
          "server": true
        }, {
          "title": "MAST",
          "info": MemeMenu.blurbs["mast"],
          "url": "tools/mast",
          "server": true
        }, {
          "title": "MCAST",
          "info": MemeMenu.blurbs["mcast"],
          "url": "tools/mcast",
          "server": true
        }, {
          "title": "GLAM2Scan",
          "info": MemeMenu.blurbs["glam2scan"],
          "url": "tools/glam2scan",
          "server": true
        }
      ]
    }, {
      "title": "Motif Comparison",
      "info": "Compare motifs.",
      "server": true,
      "topics": [
        {
          "title": "Tomtom",
          "info": MemeMenu.blurbs["tomtom"],
          "url": "tools/tomtom",
          "server": true
        }
      ]
    }, {
      "title": "Manual",
      "info": "Read (command line) help pages for all the tools in the MEME Suite. This documentation can help you understand how the web-based tools work and how to best use them.  Each help page describes the inputs and outputs of a tool when you run it from the command line after installing MEME Suite on your computer.",
      "topics": [ 
        {
          "title": "All command-line tools",
          "info": "See help pages for all the tools in the MEME Suite.",
          "url": "doc/overview.html"
        }, {
          "divider": true,
          "title": "Motif Discovery",
        }, {
          "title": "MEME",
          "info": "Read documentation on running MEME from the command-line.",
          "url": "doc/meme.html"
        }, {
          "title": "DREME",
          "info": "Read documentation on running DREME from the command-line.",
          "url": "doc/dreme.html"
        }, {
          "title": "MEME-ChIP",
          "info": "Read documentation on running MEME-ChIP from the command-line.",
          "url": "doc/meme-chip.html"
        }, {
          "title": "GLAM2",
          "info": "Read documentation on running GLAM2 from the command-line.",
          "url": "doc/glam2.html"
        }, {
          "divider": true,
          "title": "Motif Enrichment"
        }, {
          "title": "AME",
          "info": "Read documentation on running AME from the command-line.",
          "url": "doc/ame.html"
        }, {
          "title": "CentriMo",
          "info": "Read documentation on running CentriMo from the command-line.",
          "url": "doc/centrimo.html"
        }, {
          "title": "SpaMo",
          "info": "Read documentation on running SpaMo from the command-line.",
          "url": "doc/spamo.html"
        }, {
          "title": "GOMo",
          "info": "Read documentation on running GOMo from the command-line.",
          "url": "doc/gomo.html"
        }, {
          "divider": true,
          "title": "Motif Scanning"
        }, {
          "title": "FIMO",
          "info": "Read documentation on running FIMO from the command-line.",
          "url": "doc/fimo.html"
        }, {
          "title": "MAST",
          "info": "Read documentation on running MAST from the command-line.",
          "url": "doc/mast.html"
        }, {
          "title": "MCAST",
          "info": "Read documentation on running MCAST from the command-line.",
          "url": "doc/mcast.html"
        }, {
          "title": "GLAM2Scan",
          "info": "Read documentation on running GLAM2Scan from the command-line.",
          "url": "doc/glam2scan.html"
        }, {
          "divider": true,
          "title": "Motif Comparison"
        }, {
          "title": "Tomtom",
          "info": "Read documentation on running Tomtom from the command-line.",
          "url": "doc/tomtom.html"
        }
      ]
    }, {
      "title": "Guides & Tutorials",
      "info": "Find out more about how to use the MEME Suite.",
      "topics": [
        {
          "title": "DREME",
          "info": "Learn how to use DREME to find short nucleotide motifs.",
          "url": "doc/dreme-tutorial.html"
          //TODO write article
        }/*, {
          "title": "MEME-ChIP",
          "info": "Learn how to analyze large nucleotide datasets using MEME-ChIP.",
          "url": "doc/meme-chip-tutorial.html"
          //TODO Add link to Nature Protocols article
        }*/, {
          "title": "GT-Scan",
          "info": "Learn how to use GT-Scan to identify good targets for CRISPR/Cas and other genome targeting technologies.  (Note: GT-Scan is not officially part of the MEME Suite.)",
          "url": "http://crispr.braembl.org.au/gt-scan/docs/manual",
          "absolute": true
        }/*, {
          "title": "Scan a genome",
          "info": "This article will tell you how to scan genome-sized sequence sets."
          //TODO write article
        }*/
      ]
    }, {
      "title": "Sample Outputs",
      "info": "See samples of the output produced by MEME Suite programs.",
      "topics": [
        {
          "divider": true,
          "title": "Motif Discovery",
        }, {
          "title": "MEME Sample",
          "info": "See sample output from MEME.",
          "url": "doc/examples/meme_example_output_files/meme.html"
        }, {
          "title": "DREME Sample",
          "info": "See sample output from DREME.",
          "url": "doc/examples/dreme_example_output_files/dreme.html"
        }, {
          "title": "MEME-ChIP Sample",
          "info": "See sample output from MEME-ChIP.",
          "url": "doc/examples/memechip_example_output_files/index.html"
        }, {
          "title": "GLAM2 Sample",
          "info": "See sample output from GLAM2.",
          "url": "doc/examples/glam2_example_output_files/glam2.html"
        }, {
          "divider": true,
          "title": "Motif Enrichment"
        }, {
          "title": "AME Sample",
          "info": "See sample output from AME.",
          "url": "doc/examples/ame_example_output_files/ame.html"
        }, {
          "title": "CentriMo Sample",
          "info": "See sample output from CentriMo.",
          "url": "doc/examples/centrimo_example_output_files/centrimo.html"
        }, {
          "title": "SpaMo Sample",
          "info": "See sample output from SpaMo",
          "url": "doc/examples/spamo_example_output_files/spamo.html"
        }, {
          "title": "GOMo Sample",
          "info": "See sample output from GOMo.",
          "url": "doc/examples/gomo_example_output_files/gomo.html"
        }, {
          "divider": true,
          "title": "Motif Scanning"
        }, {
          "title": "FIMO Sample",
          "info": "See sample output from FIMO.",
          "url": "doc/examples/fimo_example_output_files/fimo.html"
        }, {
          "title": "MAST Sample",
          "info": "See sample output from MAST.",
          "url": "doc/examples/mast_example_output_files/mast.html"
        }, {
          "title": "MCAST Sample",
          "info": "See sample output files from MCAST.",
          "url": "doc/examples/mcast_example_output_files/mcast.html"
        }, {
          "title": "GLAM2Scan Sample",
          "info": "See sample output files from GLAM2Scan.",
          "url": "doc/examples/glam2scan_example_output_files/glam2scan.html"
        }, {
          "divider": true,
          "title": "Motif Comparison"
        }, {
          "title": "Tomtom Sample",
          "info": "See sample output files from Tomtom.  ",
          "url": "doc/examples/tomtom_example_output_files/tomtom.html"
        }
      ]
    }, {
      "title": "File Format Reference",
      "info": "Read about the file formats that the MEME Suite uses.",
      "topics": [
        {
          "title": "FASTA Sequence",
          "info": "FASTA sequence format is the main sequence format read by tools in the MEME Suite.",
          "url": "doc/fasta-format.html"
        }, {
          "title": "MEME Motif",
          "info": "MEME motif format is the main motif format read by tools in the MEME Suite.",
          "url": "doc/meme-format.html"
        }, {
          "title": "Markov Background Model",
          "info": "Background letter frequencies can be specified to MEME Suite tools using this Markov model based format.",
          "url": "doc/bfile-format.html"
        }, {
          "title": "Other Supported Formats",
          "info": "See the complete list of formats that are documented in the MEME Suite.",
          "url": "doc/overview.html#formats"
        }
      ]
    }, 
      {
      "title": "Download & Install",
      "info": "Download a copy of the MEME Suite for running on your own computer.",
      "topics": [
        {
          "title": "Download MEME Suite and Databases",
          "info": "Download a copy of the MEME Suite for local installation, as well as motif databases and GOMO databases which can be used with it.",
          "url": "doc/download.html"
        }, {
          "title": "Copyright Notice",
          "info": "Read the copyright notice.",
          "url": "doc/copyright.html"
        }, {
          "title": "Installation Guide",
          "info": "Learn how to install the MEME Suite.",
          "url": "doc/install.html"
        }, {
          "title": "Release Notes",
          "info": "Find out what was added to the MEME Suite in this and previous releases.",
          "url": "doc/release-notes.html"
        }, {
          "title": "Release Announcement Group",
          "info": "Join this GOOGLE group to be notified of new MEME Suite releases and patches.",
          "url": "https://groups.google.com/forum/#!forum/meme-suite-releases",
          "absolute": true
        }, {
          "title": "Commercial Licensing [PDF]",
          "info": "Acquire a licence for commercial use.",
          "url": "http://invent.ucsd.edu/technology/cases/2010/documents/MEME4_0_Jun_28_2011.pdf",
          "absolute": true
        }
      ]
    }, {
      "title": "Help",
      "info": "Get help using the MEME Suite.",
      "topics": [
        /*
        {
          "title": "Frequently Asked Questions",
          "info": "A list of frequently asked questions and their answers.",
          "url": "doc/general-faq.html"
          // This has been replaced by the Q&A forum
        },
        */
        {
          "title": "Q&A Forum",
          "info": "Ask questions on the forums.",
          "url": "https://groups.google.com/forum/#!forum/meme-suite",
          "absolute": true
        }, {
          "title": "Email Webmaster",
          "info": "Send the webmaster an email. This link will open your email client",
          "url": "mailto:@CONTACT@",
          "show_test": function(is_server, subs) {return subs["CONTACT"] !== ""},
          "absolute": true
        }, {
          "title": "Email Developers",
          "info": "Send the developers an email. This link will open your email client",
          "url": "mailto:@DEV_CONTACT@",
          "absolute": true
        }
      ]
    }, {
      "title": "Alternate Servers",
      "info": "List of other places that run the MEME Suite.",
      "server": true,
      "topics": [
        {
          "title": "NBCR",
          "info": "The main MEME Site site.",
          "url": "http://meme.nbcr.net",
          "absolute": true
        },
        {
          "title": "EBI (Australia)",
          "url": "http://meme.ebi.edu.au",
          "absolute": true
        },
        {
          "title": "GenQuest",
          "url": "http://tools.genouest.org/tools/meme/",
          "absolute": true
        }
      ]
    }, {
      "title": "Authors & Citing",
      "info": "Find out who contributed to the MEME Suite and how to cite the individual tools in publications.",
      "topics": [
        {
          "title": "Authors",
          "info": "The authors of the MEME Suite.",
          "url": "doc/authors.html"
        }, {
          "title": "Citing the MEME Suite",
          "info": "How to cite the MEME Suite or individual tools from the MEME Suite.",
          "url": "doc/cite.html"
        }
      ]
    }
  ],
  "opal_site": "http://opal.nbcr.net/"
};
MemeMenu.script_loaded();
