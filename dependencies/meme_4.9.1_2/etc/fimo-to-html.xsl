<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:cis="http://zlab.bu.edu/schema/cisml"
  xmlns:fimo="http://noble.gs.washington.edu/schema/cisml"
  xmlns:mem="http://noble.gs.washington.edu/meme"
>

  <xsl:output method="html" indent="yes" 
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/html4/loose.dtd"
  />

  <!-- Stylesheet processing starts here -->

  <xsl:param name="max_match_count" select="1000"/>

  <xsl:template match="/fimo">
    <html>
      <xsl:call-template name="html_head"/>
      <body bgcolor='#D5F0FF'>  
        <a name="top_buttons"></a>
        <hr />
        <table summary="buttons" align="left" cellspacing="0">
          <tr>
            <td bgcolor="#00FFFF">
              <a href='#database_and_motifs'><b>Database and Motifs</b></a>
            </td>
            <td bgcolor="#DDFFDD">
              <a href="#sec_i"><b>High-scoring Motif Occurences</b></a>
            </td>
            <td bgcolor="#DDDDFF">
              <a href="#debugging_information"><b>Debugging Information</b></a>
            </td>
          </tr>
        </table>
        <br />
        <br />
        <!-- Create the various sub-sections of the document -->
        <xsl:call-template name="version"/>
        <xsl:call-template name="database_and_motifs"/>
        <xsl:call-template name="high_scoring_motif_occurrences"/>
        <xsl:call-template name="debugging_information"/>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="html_head">
    <!-- This template prints the HTML head block, including the document level CSS. -->
    <head>
      <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1"/>
      <title>FIMO Results</title>
      <style type="text/css">
        td.left {text-align: left;}
        td.right {text-align: right; padding-right: 1cm;}
      </style>
    </head>
  </xsl:template>

  <xsl:template name="version">
    <!-- 
      This template prints the HTML describing the version of FIMO 
      that generated this document. 
    -->
    <hr />
    <center>
      <big><b>FIMO - Motif search tool</b></big>
    </center>
    <hr />
    <p>
      FIMO version <xsl:value-of select="@version"/>, 
      (Release date: <xsl:value-of select="@release"/>)
    </p>
    <p>
      For further information on how to interpret these results
      or to get a copy of the FIMO software please access
      <a href="http://meme.nbcr.net">http://meme.nbcr.net</a>
    </p>
    <p>If you use FIMO in your research, please cite the following paper:<br />
      Charles E. Grant, Timothy L. Bailey, and William Stafford Noble,
      &quot;FIMO: Scanning for occurrences of a given motif&quot;,
      <i>Bioinformatics</i>, <b>27</b>(7):1017&#45;1018, 2011.
    </p>
  </xsl:template>

  <xsl:template name="reference">
    <!-- This template prints the instructions for citing FIMO. -->
    <hr/>
    <center>
      <a name="reference"/>
      <big><b>REFERENCE</b></big>
    </center>
    <hr/>
    <p>
    If you use this program in your research, please cite:
    </p>
    <p>
    </p>
  </xsl:template>

  <xsl:template name="database_and_motifs">
    <!-- 
      This template prints the HTML descibing the input sequences
      and motifs that were used to generate this document.
     -->
    <hr/>
    <center>
      <big>
        <b><a name="database_and_motifs">DATABASE AND MOTIFS</a></b>
      </big>
    </center>
    <hr/>
    <div style="padding-left: 0.75in; line-height: 1em; font-family: monospace;">
      <p>
        DATABASE 
        <xsl:value-of select="/fimo/settings/setting[@name='sequence file name']"/>
        <br />
        Database contains 
        <xsl:value-of select="/fimo/sequence-data/@num-sequences"/>
        sequences,
        <xsl:value-of select="/fimo/sequence-data/@num-residues"/>
        residues
      </p>
      <p>
        MOTIFS 
        <xsl:value-of select="/fimo/settings/setting[@name='MEME file name']"/>
        <xsl:text> </xsl:text>
        (<xsl:value-of select="/fimo/alphabet"/>)
        <table>
          <thead>
          <tr>
            <th style="border-bottom: 1px dashed;">MOTIF</th>
            <th style="border-bottom: 1px dashed; padding-left: 1em;">WIDTH</th>
            <th style="border-bottom: 1px dashed; padding-left: 1em;" align="left">
              BEST POSSIBLE MATCH
            </th>
          </tr>
          </thead>
          <tbody>
          <xsl:for-each select="/fimo/motif">
            <tr>
              <td align="right"><xsl:value-of select="./@name"/></td>
              <td align="right" style="padding-left: 1em;"><xsl:value-of select="./@width"/></td>
              <td align="left" style="padding-left: 1em;">
                <xsl:value-of select="./@best-possible-match"/>
              </td>
            </tr>
          </xsl:for-each>
          </tbody>
        </table>
      </p>
      <p>
        Random model letter frequencies 
        (from <xsl:value-of select="/fimo/background/@source"/>):
        <br />
        <xsl:call-template name="bg_freq"/>
      </p>
    </div>
  </xsl:template>

  <xsl:template name="bg_freq">
    <!-- This template prints the HTML listing the background frequencies for each symbol in the alphabet. -->
    <xsl:for-each select="/fimo/background/value">
      <xsl:value-of select="./@letter"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="format-number(., '0.000')"/>
      <xsl:text> </xsl:text>
      <!-- For amino acids insert a line break very 9 characters. -->
      <xsl:variable name="letter_index" select="position()" />
      <xsl:if test="$letter_index mod 9 = 0">
        <xsl:text>&#10;</xsl:text>
        <br/>
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="high_scoring_motif_occurrences">
    <xsl:variable name="alphabet">
      <xsl:value-of select="/fimo/alphabet"/>
    </xsl:variable>
    <xsl:variable name="num_qvalues">
      <xsl:value-of
      select="count(document('cisml.xml')/cis:cis-element-search/cis:pattern/cis:scanned-sequence/cis:matched-element/mem:qvalue)"/>
    </xsl:variable>
    <!-- This template prints the matches to the pattern. -->
    <hr/>
    <center>
      <big><b><a name="sec_i">SECTION I: HIGH-SCORING MOTIF OCCURENCES</a></b></big>
    </center>
    <hr/>
    <ul>
    <xsl:if test="count(/fimo/incomplete-motifs) &gt; 0">
      <li>
      FIMO was configured to keep at most
      <xsl:value-of select="/fimo/settings/setting[@name='max stored scores']"/> 
      occurences for each motif in memory.
      The following motifs each had more 
      than 
      <xsl:value-of select="/fimo/settings/setting[@name='max stored scores']"/> 
      occurences with p-value less than
      <xsl:value-of select="/fimo/settings/setting[@name='output threshold']"/>.
      The less significant occurences were dropped.
      <div style="padding-left: 0.75in; line-height: 1em; font-family: monospace;">
      <table>
        <thead>
          <tr>
            <th style="border-bottom: 1px dashed;">Motif</th>
            <th style="border-bottom: 1px dashed;">Max. p-value<br />retained</th>
          </tr>
        </thead>
        <tbody>
          <xsl:for-each select="/fimo/incomplete-motifs/incomplete-motif">
            <tr>
              <td align="right"><xsl:value-of select="@name"/></td>
              <td style="padding-left: 0.5in;" align="char" char="."><xsl:value-of select="@max-pvalue"/></td>
            </tr>
          </xsl:for-each>
        </tbody>
      </table>
      </div>
      The maximum number of occurences to be kept in memory can be set using the
      <code>--max-stored-scores</code> option.
    </li>
    </xsl:if>
    <li>
    <xsl:variable name="match_count">
    <xsl:value-of select="count(document('cisml.xml')/cis:cis-element-search/cis:pattern/cis:scanned-sequence/cis:matched-element)"/>
    </xsl:variable>
    There were
    <xsl:value-of select="$match_count"/>
    motif occurences with a
    <xsl:choose>
      <xsl:when test="/fimo/settings/setting[@name='threshold type'] = 'q-value'">
        q-value less than
      </xsl:when>
      <xsl:otherwise>
        p-value less than
    </xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="/fimo/settings/setting[@name='output threshold']"/>.
    <xsl:if test="$match_count &gt; $max_match_count">
       Only the most significant 
       <xsl:value-of select="$max_match_count"/>
       are shown here.
       The full set of motif occurences can be seen in the
       tab-delimited plain text output file:
       <a href="fimo.txt">fimo.txt</a>
    </xsl:if>
    </li>
    <li> The p-value of a motif occurrence is defined as the
    probability of a random sequence of the same length as the motif
    matching that position of the sequence with as good or better a score.
    </li>
    <li> The score for the match of a position in a sequence to a motif
    is computed by summing the appropriate entries from each column of
    the position-dependent scoring matrix that represents the motif.
    </li>
    <xsl:if test="$num_qvalues > 0">
    <li> The q-value of a motif occurrence is defined as the
    false discovery rate if the occurrence is accepted as significant.
    </li>
    </xsl:if>
    <li>The table is sorted by increasing p-value.</li>
    </ul>
    <table border="1">
      <thead>
        <tr>
          <th>Motif</th>
          <th>Sequence Name</th>
          <th>Strand</th>
          <th>Start</th>
          <th>End</th>
          <th>p-value</th>
          <xsl:if test="$num_qvalues > 0">
            <th>q-value</th>
          </xsl:if>
          <th>Matched Sequence</th>
        </tr>
      </thead>
      <tbody>
        <xsl:for-each select="document('cisml.xml')/cis:cis-element-search/cis:pattern/cis:scanned-sequence/cis:matched-element">
          <xsl:sort select="@pvalue" data-type="number" order="ascending"/>
          <xsl:variable name="seq_name" select="../@name"/>
          <xsl:if test="position() &lt;= $max_match_count">
            <tr>
              <td align="left">
               <a name="occurrences_{$seq_name}"><xsl:value-of select="../../@name"/></a>
              </td>
              <td align="left">
               <xsl:value-of select="../@name"/>
              </td>
              <!-- strand, start  and stop positions -->
              <!-- CISML uses start > stop to indicate - strand -->
              <!-- For HTML we want start < stop, indicate strand in strand column -->
              <xsl:choose>

                <xsl:when test="@start &gt; @stop">
                  <!-- start > stop so - strand -->
                  <td align="center"><xsl:text>&#8722;</xsl:text></td>
                </xsl:when>

                <xsl:otherwise>

                  <xsl:choose>
                    <xsl:when test="$alphabet='nucleotide'">
                     <td align="center"><xsl:text>+</xsl:text></td>
                    </xsl:when>

                    <xsl:otherwise>
                     <td align="center"><xsl:text>.</xsl:text></td>
                    </xsl:otherwise>
                  </xsl:choose>

                </xsl:otherwise>

              </xsl:choose>
              <xsl:choose>
                <xsl:when test="@start &gt; @stop">
                  <!-- start > stop so - strand -->
                  <td align="left">
                    <xsl:value-of select="@stop"/>
                  </td>
                  <td align="left">
                    <xsl:value-of select="@start"/>
                  </td>
                </xsl:when>
                <xsl:otherwise>
                  <td align="left">
                    <xsl:value-of select="@start"/>
                  </td>
                  <td align="left">
                    <xsl:value-of select="@stop"/>
                  </td>
                </xsl:otherwise>
              </xsl:choose>
              <td align="left">
               <xsl:value-of select="@pvalue"/>
              </td>
              <xsl:if test="$num_qvalues > 0">
                <td align="left">
                 <xsl:value-of select="./mem:qvalue"/>
                </td>
              </xsl:if>
              <td align="left">
               <code style="font-size: x-large;"><xsl:value-of select="./cis:sequence"/></code>
              </td>
            </tr>
          </xsl:if>
        </xsl:for-each>
      </tbody>
    </table>
  </xsl:template>

  <xsl:template name="debugging_information">
    <!-- This template print the HTML describing the FIMO input parameters. -->
    <hr/>
    <center>
      <big><b><a name="debugging_information">DEBUGGING INFORMATION</a></b></big>
    </center>
    <hr/>
    <p>
    Command line:
    </p>
    <pre>
    <xsl:value-of select="/fimo/command-line"/>
    </pre>
    <p>
    Settings:
    </p>
    <pre>
    <table>
    <xsl:apply-templates select="/fimo/settings/setting[position() mod 3 = 1]"/>
    </table>
    </pre>
    <p>
      This information can be useful in the event you wish to report a
      problem with the FIMO software.
    </p>
    <hr />
    <span style="background-color: #DDDDFF"><a href="#top_buttons"><b>Go to top</b></a></span>
  </xsl:template>

  <xsl:template match="setting">
    <!-- This template prints the program settings in 2 columns -->
    <tr>
      <td style="padding-right: 2em"><xsl:value-of select="@name"/> = <xsl:value-of select="."/></td>
      <xsl:choose>
      <xsl:when test="count(following-sibling::*[name()=name(current())])">
        <td style="padding-left: 5em; padding-right: 2em">
          <xsl:value-of select="following-sibling::setting[1]/@name"/> = <xsl:value-of select="following-sibling::setting[1]"/>
        </td>
        <td style="padding-left: 5em; padding-right: 2em">
          <xsl:value-of select="following-sibling::setting[2]/@name"/> = <xsl:value-of select="following-sibling::setting[2]"/>
        </td>
      </xsl:when>
      <xsl:otherwise>
        <td style="padding-left: 5em; padding-right: 2em"></td>
        <td align="right"></td>
      </xsl:otherwise>
      </xsl:choose>
    </tr>
  </xsl:template>

</xsl:stylesheet>

