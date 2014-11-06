<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template name="meme.css">
    <![CDATA[
    /* START INCLUDED FILE "meme.css" */
        /* The following is the content of meme.css */
        body { background-color:white; font-size: 12px; font-family: Verdana, Arial, Helvetica, sans-serif;}

        div.help {
          display: inline-block;
          margin: 0px;
          padding: 0px;
          width: 12px;
          height: 13px;
          cursor: pointer;
          background-image: url("help.gif");
          background-image: url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAANAQMAAACn5x0BAAAAAXNSR0IArs4c6QAAAAZQTFRFAAAAnp6eqp814gAAAAF0Uk5TAEDm2GYAAAABYktHRACIBR1IAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH2gMJBQgGYqhNZQAAACZJREFUCNdj+P+BoUGAoV+AYeYEEGoWYGgTYGgRAAm2gRGQ8f8DAOnhC2lYnqs6AAAAAElFTkSuQmCC");
        }

        div.help2 {
          color: #999;
          display: inline-block;
          width: 12px;
          height: 12px;
          border: 1px solid #999;
          font-size: 13px;
          line-height:12px;
          font-family: Helvetica, sans-serif;
          font-weight: bold;
          font-style: normal;
          cursor: pointer;
        }
        div.help2:hover {
          color: #000;
          border-color: #000;
        }
        
        p.spaced { line-height: 1.8em;}
        
        span.citation { font-family: "Book Antiqua", "Palatino Linotype", serif; color: #004a4d;}

        p.pad { padding-left: 30px; padding-top: 5px; padding-bottom: 10px;}

        td.jump { font-size: 13px; color: #ffffff; background-color: #00666a;
          font-family: Georgia, "Times New Roman", Times, serif;}

        a.jump { margin: 15px 0 0; font-style: normal; font-variant: small-caps;
          font-weight: bolder; font-family: Georgia, "Times New Roman", Times, serif;}

        h2.mainh {font-size: 1.5em; font-style: normal; margin: 15px 0 0;
          font-variant: small-caps; font-family: Georgia, "Times New Roman", Times, serif;}

        h2.line {border-bottom: 1px solid #CCCCCC; font-size: 1.5em; font-style: normal;
          margin: 15px 0 0; padding-bottom: 3px; font-variant: small-caps;
          font-family: Georgia, "Times New Roman", Times, serif;}

        h4 {border-bottom: 1px solid #CCCCCC; font-size: 1.2em; font-style: normal;
          margin: 10px 0 0; padding-bottom: 3px; font-family: Georgia, "Times New Roman", Times, serif;}

        h5 {margin: 0px}

        a.help { font-size: 9px; font-style: normal; text-transform: uppercase;
          font-family: Georgia, "Times New Roman", Times, serif;}

        div.pad { padding-left: 30px; padding-top: 5px; padding-bottom: 10px;}
        
        div.pad1 { margin: 10px 5px;}

        div.pad2 { margin: 25px 5px 5px;}
        h2.pad2 { padding: 25px 5px 5px;}

        div.pad3 { padding: 5px 0px 10px 30px;}

        div.box { border: 2px solid #CCCCCC; padding:10px;}

        div.bar { border-left: 7px solid #00666a; padding:5px; margin-top:25px; }

        div.subsection {margin:25px 0px;}

        img {border:0px none;}

        th.majorth {text-align:left;}
        th.minorth {font-weight:normal; text-align:left; width:8em; padding: 3px 0px;}
        th.actionth {font-weight:normal; text-align:left;}

        .strand_name {text-align:left;}
        .strand_side {padding:0px 10px;}
        .strand_start {padding:0px 10px;}
        .strand_pvalue {text-align:center; padding:0px 10px;}
        .strand_lflank {text-align:right; padding-right:5px; font-weight:bold; font-size:large; font-family: 'Courier New', Courier, monospace; color:gray;}
        .strand_seq {text-align:center; font-weight:bold; font-size:large; font-family: 'Courier New', Courier, monospace;}
        .strand_rflank {text-align:left; padding-left:5px; font-weight:bold; font-size:large; font-family: 'Courier New', Courier, monospace; color:gray;}

        .block_td {height:25px;}
        .block_container {position:relative; width:98%; height:25px; padding:0px; margin: 0px 0px 0px 1em;}
        .block_motif {position:absolute; z-index:3; height:12px; top:0px; text-align:center; vertical-align:middle; background-color:cyan;}
        .block_rule {position:absolute; z-index:2; width:100%; height:1px; top:12px; left:0px; background-color:gray;}
        .block_plus_sym {position:absolute; z-index:4; line-height:12px; top:0px; left:-1em;}
        .block_minus_sym {position:absolute; z-index:4; line-height:12px; top:13px; left:-1em;}

        .tic_major {position:absolute; border-left:2px solid blue; height:0.5em; top:0em;}
        .tic_minor {position:absolute; border-left:1px solid blue; height:0.2em; top:0em;}
        .tic_label {position:absolute; top:0.5em;  height: 1em; text-align:center; vertical-align:middle}

        .explain h5 {font-size:1em; margin-left: 1em;}

        div.doc {margin-left: 2em; margin-bottom: 3em;}
        
        div.tabArea {
          font-size: 80%;
          font-weight: bold;
        }

        a.tab {
          background-color: #ddddff;
          border: 1px solid #000000;
          padding: 2px 1em 2px 1em;
          text-decoration: none;
        }
        div.tabArea.base a.tab {
          border-top-width: 0px;
        }
        div.tabArea.top a.tab {
          border-bottom-width: 0px;
        }

        a.tab, a.tab:visited {
          color: #808080;
        }

        a.tab:hover {
          background-color: #d0d0d0;
          color: #606060;
        }
        a.tab.activeTab, a.tab.activeTab:hover, a.tab.activeTab:visited {
          background-color: #f0f0f0;
          color: #000000;
        }
        div.tabMain {
          border: 1px solid #000000;
          background-color: #ffffff;
          padding: 5px;
          margin-right: 5px;
        }
        th.trainingset {
          border-bottom: thin dashed black; 
          font-weight:normal; 
          padding:0px 10px;
        }
        .dnaseq {
          font-weight: bold; 
          font-size: large; 
          font-family: 'Courier New', Courier, monospace;
        }
        .dna_A {
          color: rgb(204,0,0);
        }
        .dna_C {
          color: rgb(0,0,204);
        }
        .dna_G {
          color: rgb(255,179,0);
        }
        .dna_T {
          color: rgb(0,128,0);
        }
    /* END INCLUDED FILE "meme.css" */
    ]]>
  </xsl:template>
</xsl:stylesheet>

