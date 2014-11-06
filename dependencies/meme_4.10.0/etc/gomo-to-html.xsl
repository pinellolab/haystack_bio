<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [<!ENTITY nbsp "&#160;"> <!ENTITY eacute "&#233;">]>
<!-- define nbsp & eacute as it is not defined in xml, only lt, gt and amp are defined by default -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:variable name="show_details" select="not (/gomo/motif[goterm][1]/goterm[1]/@name = '')" />
  <xsl:variable name="show_logos" select="/gomo/motif[@logo][1]/@logo" />

<!-- 
  This file is automatically built from gomo.xsl.in at MAKE time.

  This stylesheet transforms the XML output of GOMo into HTML.
-->

  <xsl:output method="html" indent="yes" 
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/html4/loose.dtd"
  />
  <!-- contains constant site_url and amigo_url -->
  <xsl:include href="constants.xsl"/>
  <xsl:include href="meme.css.xsl"/>
  <xsl:include href="utilities.js.xsl"/>
  <xsl:variable name="amigo_url_before" select="substring-before($amigo_url, 'GO_TERM_ID')"/>
  <xsl:variable name="amigo_url_after" select="substring-after($amigo_url, 'GO_TERM_ID')"/>

  <xsl:strip-space elements="*"/>

  <!-- Stylesheet processing starts here -->
  <xsl:template match="/gomo">
    <html>
      <xsl:call-template name="html-head"/>
      <body bgcolor="#ffffff">
        <!-- Create the various sub-sections of the document -->
        <xsl:call-template name="help-popups"/>
        <xsl:call-template name="top"/>
        <xsl:call-template name="overview"/>
        <xsl:call-template name="motifs"/>
        <xsl:call-template name="info"/>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="html-head">
    <!-- This template prints the HTML head block, including the document level CSS. -->
    <head>
      <meta charset="UTF-8"/>
      <title>GOMo Results</title>
      <xsl:call-template name="html-css"/>
      <xsl:call-template name="html-js"/>
    </head>
  </xsl:template>
  
  <xsl:template name="html-css">
    <style type="text/css">
        <xsl:call-template name="meme.css"/>
        body { font-size: 12px; font-family: Verdana, Arial, Helvetica, sans-serif;}

        a.jump { margin: 15px 0 0; font-style: normal; font-variant: small-caps;
          font-weight: bolder; font-family: Georgia, "Times New Roman", Times, serif;}

        h2.mainh { font-size: 1.5em; font-style: normal; margin: 15px 0 0;
          font-variant: small-caps; font-family: Georgia, "Times New Roman", Times, serif;}

        h2.line { border-bottom: 1px solid #CCCCCC; font-size: 1.5em; font-style: normal;
          margin: 15px 0 0; padding-bottom: 3px; font-variant: small-caps;
          font-family: Georgia, "Times New Roman", Times, serif;}

        h4 { border-bottom: 1px solid #CCCCCC; font-size: 1.2em; font-style: normal;
          margin: 10px 0 0; padding-bottom: 3px; 
          font-family: Georgia, "Times New Roman", Times, serif;}

        a.help { font-size: 9px; font-style: normal; text-transform: uppercase;
          font-family: Georgia, "Times New Roman", Times, serif;}

        div.pad1 { margin: 10px 5px;}

        div.pad2 { margin: 25px 5px 5px;}

        div.pad3 { padding: 5px 0px 10px 30px;}

        div.box { border: 2px solid #CCCCCC; padding:10px;}

        div.bar { border-left: 7px solid #00666a; padding:5px; }

        img { border: none; border-width: 0px;}

        .implied a:link { color: darkslategrey;}

        .implied a:visited { color: darkslateblue;}

        tr.normal { color: black; border-bottom: 1px solid #CCCCCC;}

        tr.implied { color: grey; border-bottom: 1px solid #CCCCCC;}

        .bp { color: #770000;}

        .cc { color: #CC6600;}

        .mf { color: #007700;}

        col.em25 {width: 25em;}

        col.em7 {width: 7em;}

        col.em6 {width: 6em;}

        col.em2 {width: 2em;}

        textarea.cmd {width: 100%;}

        .explain h5 {margin-left: 1em;}

        .explain p {margin-left: 2em;}

    </style>
  </xsl:template>

  <xsl:template name="html-js">
    <script type="text/javascript">
      <xsl:call-template name="utilities.js" />
    </script>
  </xsl:template>

  <xsl:template name="help-popups">
    <div class="pop_content" id="pop_motif">
      <p>The name (or number) identifying the motif in the input files.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_logo">
      <p>The visual representation of the position weight matrix otherwise known as the motif logo.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_predictions">
      <p>The number of term predictions.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_bestpred">
      <p>If specificity data is avaliable then this displays the top 5 specific predictions, otherwise it just
        displays the top 5 predictions.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_goterm">
      <p>The Gene Ontology Consortium term for a specific role or locality. Used for annotating genes with their purposes.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_gscore">
      <p>A score generated as the <a href="http://en.wikipedia.org/wiki/Geometric_mean" >geometric mean</a> 
        of <a href="http://en.wikipedia.org/wiki/Mann%E2%80%93Whitney_U" >rank-sum test(s)</a> for the particular Gene Ontology term.
        The two groups compared by the rank-sum test are scores of genes annotated with the GO term and scores of genes
        not annotated with the GO term.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_epvalue">
      <p>An empirically generated <i>p</i>-value. The null hypothesis is that by shuffling the labels on gene scores, 
        any possible association between the set of genes that a GO term annotates is destroyed. A large number of scores
        are generated using the null hypothesis and the number of null hypotheis scores that are better than each of the 
        real scores is summed and then divided by the total null hypothesis scores generated to calculate a <i>p</i>-value.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_eqvalue">
      <p>An empirically generated <i>q</i>-value. The <i>q</i>-values are calculated from the <i>p</i>-values using the 
        method proposed by Benjamini &amp; Hochberg (1995).</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_specif">
      <p>A measure of the relative position in the GO hierarchy showing how specific a term is. A value of 100% implies that
        the GO term is the most specific form whereas a value of 0% implies that the GO term is one of the three roots of
        the Gene Ontology with only terms below it. A tilda (~) infront of the percentage implies that the value shown has 
        been rounded and is not exact.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_goname">
      <p>The Gene Ontology hierarchy and Gene Ontology name of the term. The hierarchy is specified by the abbreviations 
        <span class="bp">BP</span> for biological process,
        <span class="cc">CC</span> for cellular component and
        <span class="mf">MF</span> for molecular function.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_generank">
      <p>Gene name and rank of (up to) top 10 genes annotated with the GO term for the primary species.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
  </xsl:template>
  
  <xsl:template name="top">
    <div class="pad1">
      <div class="prog_logo big">
        <img src="{$site_url}/doc/images/gomo_icon.png" alt="GOMo Logo"/>
        <h1>GOMo</h1>
        <h2>Gene Ontology for Motifs</h2>
      </div>
      <p>
        For further information on how to interpret these results or to get a 
        copy of the MEME software please access<a href="http://meme.sdsc.edu">
        http://meme.sdsc.edu</a>.<br />
      </p>
      <p>If you use GOMo in your research, please cite the following paper:<br />
        <span class="citation">
          Fabian A. Buske, Mikael Bod&eacute;n, Denis C. Bauer and Timothy L. Bailey,
          &quot;Assigning roles to DNA regulatory motifs using comparative genomics&quot;,
          <i>Bioinformatics</i>, <b>26</b>(7), 860-866, 2010.
          <a href="http://bioinformatics.oxfordjournals.org/content/26/7/860.full">[full text]</a>
        </span>
      </p>
    </div>

    <a name="top_buttons"/>
    <div class="pad2">
      <a class="jump" href="#overview">Investigated Motifs</a>
      &nbsp;&nbsp;|&nbsp;&nbsp;
      <a class="jump" href="#version">Program information</a>
    </div>
  </xsl:template>


  <xsl:template name="overview">
    <xsl:variable name="pred-display" select="5" />
    <a name="overview"/>
    <h2 class="mainh pad2">Investigated Motifs</h2>
    <div class="box" >
      <h4>Overview</h4>
      <div class="pad3">
        <table border="0" cellspacing="5" cellpadding="0" bgcolor="#ffffff">
          <thead>
          <tr>
            <th>Motif&nbsp;<div class="help" onclick="help_popup(this,'pop_motif')"></div></th>
            <xsl:if test="$show_logos"><th>Logo&nbsp;<div class="help" onclick="help_popup(this,'pop_logo')"></div></th></xsl:if>
            <th>Predictions&nbsp;<div class="help" onclick="help_popup(this,'pop_predictions')"></div></th>
            <th>Top&nbsp;<xsl:value-of select="$pred-display" /> 
              <xsl:if test="$show_details">&nbsp;specific</xsl:if>&nbsp;predictions&nbsp;<div class="help" onclick="help_popup(this,'pop_bestpred')"></div></th>
          </tr>
          </thead>
          <tbody>
          <xsl:for-each select="/gomo/motif">
            <xsl:variable name="motif" select="@id" />
            <xsl:variable name="logofile" select="@logo" />
            <tr bgcolor="#cccccc" height="1">
              <td colspan="4"></td>
            </tr>
            <tr>
              <td><a href="#motif_{$motif}"><xsl:value-of select="@id" /></a></td>
              <xsl:if test="$show_logos" >
                <td>
                  <a href="#motif_{$motif}">
                    <img src="res/{$logofile}" height="80" alt="Missing Motif {$motif} Logo" title="Motif {$motif} Logo" border="0"/>
                  </a>
                </td>
              </xsl:if>
              <td align="center"><a href="#motif_{$motif}"><xsl:value-of select="count(goterm)"/></a></td>
              <td>
                <xsl:for-each select="goterm[@implied = 'n' or @implied = 'u'][position() &lt; ($pred-display + 1)]">
                  <xsl:choose>
                    <xsl:when test="$show_details">
                      <xsl:choose>
                        <xsl:when test="@group = 'b'">
                          <span class="bp">BP&nbsp;</span>
                        </xsl:when>
                        <xsl:when test="@group = 'c'">
                          <span class="cc">CC&nbsp;</span>
                        </xsl:when>
                        <xsl:when test="@group = 'm'">
                          <span class="mf">MF&nbsp;</span>
                        </xsl:when>
                        <xsl:otherwise>
                          <xsl:value-of select="@group"/>&nbsp;
                        </xsl:otherwise>
                      </xsl:choose>
                      <xsl:value-of select="@name"/>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="@id"/>
                    </xsl:otherwise>
                  </xsl:choose>
                  <br />
                </xsl:for-each>
              </td>
            </tr>
          </xsl:for-each>
          </tbody>
        </table>
      </div>
    </div>
    <br />
    <br />
  </xsl:template>



  <xsl:template name="motifs">
    <xsl:for-each select="/gomo/motif">
      <xsl:call-template name="motif" />
    </xsl:for-each>
  </xsl:template>
  
  <xsl:template name="motif">
    <xsl:variable name="prev_motif" select="preceding-sibling::motif[1]/@id"/>
    <xsl:variable name="next_motif" select="following-sibling::motif[1]/@id"/>
    <xsl:variable name="motif" select="@id" />

    <!-- There is no elegent way to remove this layout table -->
    <a name="motif_{$motif}"/>
    <table width="100%" border="0" cellspacing="1" cellpadding="4" bgcolor="#FFFFFF">
      <tr>
        <td>
          <h2 class="mainh">Motif <xsl:value-of select="@id" /></h2>
        </td>
        <td align="right" valign="bottom">
          <xsl:if test="$prev_motif"><a href="#motif_{$prev_motif}">Previous</a>&nbsp;</xsl:if>
          <xsl:if test="$next_motif"><a href="#motif_{$next_motif}">Next</a>&nbsp;</xsl:if>
          <a href="#top_buttons">Top</a>
        </td>
      </tr>
    </table>

    <div class="box" >
      <xsl:choose>
      <xsl:when test="goterm">
        <xsl:if test="$show_details">
          GO terms are shown in grey if a more specific GO term was also significantly associated with this motif. 
          The most specific GO terms are shown in black.<br />
          <span class="bp">BP</span> stands for biological process, 
          <span class="cc">CC</span> stands for cellular component and 
          <span class="mf">MF</span> stands for molecular function.<br />
          <br />
        </xsl:if>
        <table width="100%" cellpadding="0" cellspacing="2">
          <col class="em6"/> 
          <col class="em7"/>
          <col class="em6"/>
          <col class="em6"/>
          <xsl:if test="$show_details">
            <col class="em7"/>
            <col class="em2"/>
            <col class="em25"/>
          </xsl:if>
          <col />
          <thead>
            <tr bgcolor="#ffffff" valign="top">
              <th>GO&nbsp;term&nbsp;<div class="help" onclick="help_popup(this,'pop_goterm')"></div></th>
              <th>score&nbsp;<div class="help" onclick="help_popup(this,'pop_gscore')"></div></th>
              <th><i>p</i>-value&nbsp;<div class="help" onclick="help_popup(this,'pop_epvalue')"></div></th>
              <th><i>q</i>-value&nbsp;<div class="help" onclick="help_popup(this,'pop_eqvalue')"></div></th>
              <xsl:if test="$show_details" >
                <th nowrap="nowrap">Specificity&nbsp;<div class="help" onclick="help_popup(this,'pop_specif')"></div></th>
                <th colspan="2" >GO&nbsp;name&nbsp;<div class="help" onclick="help_popup(this,'pop_goname')"></div></th>
              </xsl:if>
              <th>Gene&nbsp;ID&nbsp;/&nbsp;Rank (<xsl:value-of select="@genecount"/> genes in total) 
                <div class="help" onclick="help_popup(this,'pop_generank')"></div>
              </th>
            </tr>
          </thead>
          <tbody>
            <xsl:for-each select="goterm">
              <xsl:choose>
              <xsl:when test="@implied = 'y'">
                <xsl:call-template name="goterm">
                  <xsl:with-param name="implied" select="'implied'" />
                </xsl:call-template>
              </xsl:when>
              <xsl:otherwise>
                <xsl:call-template name="goterm">
                  <xsl:with-param name="implied" select="'normal'" />
                </xsl:call-template>
              </xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
          </tbody>
        </table>
      </xsl:when>
      <xsl:otherwise>
        <i>... no significant GO-term could be associated with this motif ...</i><br/>
      </xsl:otherwise>
      </xsl:choose>
    </div>
    <br />
  </xsl:template>  
    
  <xsl:template name="goterm">
    <xsl:param name="implied" />
    <tr valign="top" class="{$implied}">
      <td align="left">
        <a href="{concat($amigo_url_before, @id, $amigo_url_after)}" target="go_query"><xsl:value-of select="@id"/></a>
      </td>
      <td align="center"><xsl:value-of select="@score"/></td>
      <td align="center"><xsl:value-of select="@pvalue"/></td>
      <td align="center"><xsl:value-of select="@qvalue"/></td>
      <xsl:if test="$show_details" >
      <td align="center">
        <xsl:variable name="unrounded_specificity" select="(@nabove div (@nabove + @nbelow)) *100" />
        <xsl:variable name="rounded_specificity" select="round($unrounded_specificity)" />
        <xsl:if test="$rounded_specificity != $unrounded_specificity">~</xsl:if><xsl:value-of select="$rounded_specificity"/>%
      </td>
      <td align="left">
        <xsl:choose>
          <xsl:when test="@group = 'b'">
            <span class="bp">BP&nbsp;</span>
          </xsl:when>
          <xsl:when test="@group = 'c'">
            <span class="cc">CC&nbsp;</span>
          </xsl:when>
          <xsl:when test="@group = 'm'">
            <span class="mf">MF&nbsp;</span>
          </xsl:when>
          <xsl:when test="@group = 'obsolete'">
            <span class="obsolete">??</span>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="@group"/>&nbsp;
          </xsl:otherwise>
        </xsl:choose>
      </td>
      <td>
        <xsl:if test="@group = 'obsolete'">
          <span class="obsolete">GO term is obsolete. Please report this to an administrator so the sequence database can be updated.</span>
        </xsl:if>
        <xsl:value-of select="@name"/>
      </td>
      </xsl:if>
      <td>
        <xsl:for-each select="gene" >
          <xsl:choose>
            <xsl:when test="/gomo/program/@gene_url">
              <xsl:variable name="base_url" select="/gomo/program/@gene_url"/>
              <xsl:variable name="gene_url" select="concat(substring-before($base_url, '!!GENEID!!'), @id, substring-after($base_url, '!!GENEID!!'))" />
              <a href="{$gene_url}" target="gene_query" ><xsl:value-of select="@id"/>&nbsp;(<xsl:value-of select="@rank"/>)</a>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="@id"/>&nbsp;(<xsl:value-of select="@rank"/>)
            </xsl:otherwise>
          </xsl:choose>
          <xsl:choose >
            <xsl:when test="position()!=last()">, </xsl:when>
            <xsl:when test="../@annotated &gt; 10">, ...<xsl:value-of select="../@annotated - 10"/> more...</xsl:when>
          </xsl:choose>
        </xsl:for-each>
      </td>
    </tr>

  </xsl:template>

  <xsl:template name="info">
    <div class="bar">
      <a name="version"/>
      <b>GOMo version:</b><br />
      <xsl:value-of select="/gomo/@version" /> (Release date: <xsl:value-of select="/gomo/@release" />)<br />
      <br />
      <a name="reference"/>
      <b>Reference:</b><br />
      <div class="cite">
        Mikael Bod&eacute;n and Timothy L. Bailey,
        &quot;Associating transcription factor binding site motifs with target Go terms and target genes&quot;, 
        <i>Nucleic Acids Research</i>, <b>36</b>, 4108-4117, 2008. 
        <a href="http://nar.oxfordjournals.org/cgi/content/abstract/gkn374v1">[full text]</a> 
      </div>
      <br />
      <div class="cite">
        Fabian A. Buske, Mikael Bod&eacute;n, Denis C. Bauer and Timothy L. Bailey,
        &quot;Assigning roles to DNA regulatory motifs using comparative genomics&quot;,
        <i>Bioinformatics</i>, <b>26</b>, 860-866, 2010.
        <a href="http://bioinformatics.oxfordjournals.org/cgi/content/full/26/7/860">[full text]</a>
      </div>      
      <br />
      <a name="sequences"/>
      <b>Input Data: </b><br />
      go-term-sequence mapping: <xsl:value-of select="/gomo/program/gomapfile/@path"/><br />
      <xsl:for-each select="/gomo/program/seqscorefile">
        scored sequence file: <xsl:value-of select="@path"/><br />
      </xsl:for-each>
      <br />
      <a name="command"/>
      <b>Command line summary:</b><br />
      This information can also be useful in the event you wish to report
      a problem with the GOMo software.<br />
      <br />
      Command:<br />
      <textarea rows="2" class="cmd" readonly="readonly" >
        <xsl:value-of select="/gomo/program/@cmd"/>
      </textarea><br />
      <br />
      Seed: <xsl:value-of select="/gomo/program/@seed"/><br/>
      <br/>
      Significance Threshold: <xsl:value-of select="/gomo/program/@q_threshold"/><br/>        
    </div>
  </xsl:template>

  <xsl:template name="explanation">
    <span class="explain">
    <a name="explanation"></a>
    <h2>Explanation of GOMo results</h2>
    <div class="box">
      <h4>The GOMo results consist of</h4>
      <ul style="margin-left: 2em; padding-left: .5em;">
        <li>
          The result of the <a href="#overview"><b>Motifs</b></a> used to search
          for GO terms associated with putative target genes.  
        </li>
        <li>
          The <a href="#version"><b>version</b></a>
           of GOMo.
        </li>
        <li>
          The <a href="#reference"><b>reference</b></a>
          to cite if you use GOMo in your research.
        </li>
        <li>
          The list of <a href="#sequences"><b>input data</b></a> used for the 
          the calculations.
        </li>
        <li>
          The <a href="#command"><b>command line summary</b></a> 
          detailing the parameters with which you ran GOMo.
        </li>
      </ul>
      <h4>Motif Overview Column Descriptions</h4>
      <h5><a name="motif">Motif</a></h5>
      <p>The name (or number) identifying the motif in the input files.</p>
      <h5><a name="logo">Logo</a></h5>
      <p>The visual representation of the position weight matrix otherwise known as the motif logo.</p>
      <h5><a name="predictions">Predictions</a></h5>
      <p>The number of term predictions.</p>
      <h5><a name="bestpred">Top 5 (specific) predictions</a></h5>
      <p>If specificity data is avaliable then this displays the top 5 specific predictions, otherwise it just
        displays the top 5 predictions.</p>
      <h4>Motif Results Column Descriptions</h4>
      <h5><a name="goterm">GO term</a></h5>
      <p>The Gene Ontology Consortium term for a specific role or locality. Used for annotating genes with their purposes.</p>
      <h5><a name="gscore">GOMo score</a></h5>
      <p>A score generated as the <a href="http://en.wikipedia.org/wiki/Geometric_mean" >geometric mean</a> 
        of <a href="http://en.wikipedia.org/wiki/Mann%E2%80%93Whitney_U" >rank-sum test(s)</a> for the particular Gene Ontology term.
        The two groups compared by the rank-sum test are scores of genes annotated with the GO term and scores of genes
        not annotated with the GO term.</p>
      <h5><a name="epvalue">Empirical <i>p-value</i></a></h5>
      <p>An empirically generated <i>p-value</i>. The null hypothesis is that by shuffling the labels on gene scores, 
        any possible association between the set of genes that a GO term annotates is destroyed. A large number of scores
        are generated using the null hypothesis and the number of null hypotheis scores that are better than each of the 
        real scores is summed and then divided by the total null hypothesis scores generated to calculate a <i>p-value</i>.
      </p>
      <h5><a name="eqvalue">Empirical <i>q-value</i></a></h5>
      <p>An empirically generated <i>q-value</i>. The <i>q-values</i> are calculated from the <i>p-values</i> using the 
        method proposed by Benjamini &amp; Hochberg (1995).
      </p>
      <h5><a name="specif">Specificity</a></h5>
      <p>A measure of the relative position in the GO hierarchy showing how specific a term is. A value of 100% implies that
        the GO term is the most specific form whereas a value of 0% implies that the GO term is one of the three roots of
        the Gene Ontology with only terms below it. A tilda (~) infront of the percentage implies that the value shown has 
        been rounded and is not exact.
      </p>
      <h5><a name="goname">GO Name</a></h5>
      <p>The Gene Ontology hierarchy and Gene Ontology name of the term. The hierarchy is specified by the abbreviations 
        <span class="bp">BP</span> for biological process,
        <span class="cc">CC</span> for cellular component and
        <span class="mf">MF</span> for molecular function.
      </p>
      <h5><a name="generank">Gene ID / Rank</a></h5>
      <p>Gene name and rank of (up to) top 10 genes annotated with the GO term for the primary species.</p>
    </div>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    <br/>
    </span>
  </xsl:template>

</xsl:stylesheet>
