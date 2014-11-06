<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [
<!ENTITY nbsp "&#160;">
<!ENTITY space " ">
<!ENTITY newline "&#10;">
<!ENTITY tab "&#9;">
<!ENTITY more "&#8615;">
]><!-- define nbsp as it is not defined in xml, only lt, gt and amp are defined by default -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="html" indent="yes" 
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/html4/loose.dtd"
    />
  <xsl:include href="constants.xsl"/>
  <xsl:include href="meme.css.xsl"/>
  <xsl:include href="mast-to-html.css.xsl"/>
  <xsl:include href="utilities.js.xsl"/>
  <xsl:include href="mast-to-html.js.xsl"/>
  <xsl:include href="block-diagram.xsl"/>

  <!-- useful variables -->
  <!-- unfortunately I can't remove this calculation,
       maybe I should do this in c and store it? -->
  <xsl:variable name="max_seq_len">
    <xsl:for-each select="/mast/sequences/sequence" >
      <xsl:sort select="@length" data-type="number" order="descending"/>
      <xsl:if test="position() = 1">
        <xsl:value-of select="@length" />
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="max_log_pvalue">
    <xsl:call-template name="ln-approx">
      <xsl:with-param name="base" select="0.0000000001"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- unfortunately I can't remove this calculcation,
       maybe I should do this in C and store it? -->
  <xsl:variable name="longest_seq_name">
    <xsl:for-each select="/mast/sequences/sequence" >
      <xsl:sort select="string-length(@name)" data-type="number" order="descending"/>
      <xsl:if test="position() = 1">
        <xsl:value-of select="string-length(@name)" />
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="seq_name_col_size">
    <xsl:choose>
      <xsl:when test="$longest_seq_name &lt; 10">
        <xsl:text>8</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="round($longest_seq_name * 0.8)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>


  <!-- Stylesheet processing starts here -->
  <xsl:template match="/">
    <!-- Possible way to output a html 5 doctype -->
    <!--
    <xsl:text disable-output-escaping='yes'>&lt;!DOCTYPE HTML&gt;&newline;</xsl:text>
    -->
    <html>
      <xsl:call-template name="html-head"/>
      <xsl:call-template name="html-body"/>
    </html>
  </xsl:template>

  <xsl:template name="html-head">
    <head>
      <meta charset="UTF-8"/>
      <meta name="description" content="Motif Alignment and Search Tool (MAST) output."/>
      <title>MAST</title>
      <xsl:call-template name="html-css"/>
      <xsl:call-template name="html-js"/>
    </head>
  </xsl:template>

  <xsl:template name="html-css">
      <style type="text/css">
        <xsl:call-template name="meme.css" />
        <xsl:call-template name="mast-to-html.css"/>
      </style>
  </xsl:template>

  <xsl:template name="html-js">
      <script type="text/javascript">
      <xsl:call-template name="utilities.js" />
        var motifs = new Array();<xsl:text>&newline;</xsl:text>
        <xsl:for-each select="/mast/motifs/motif">
          <xsl:text>        motifs[&quot;</xsl:text><xsl:value-of select='@id'/><xsl:text>&quot;] = new Motif(&quot;</xsl:text>
          <xsl:value-of select='@name'/><xsl:text>&quot;, &quot;</xsl:text>
          <xsl:value-of select='/mast/alphabet/@type'/><xsl:text>&quot;, &quot;</xsl:text>
          <xsl:value-of select='@best_f'/><xsl:text>&quot;, &quot;</xsl:text>
          <xsl:value-of select='@best_r' /><xsl:text>&quot;);&newline;</xsl:text>
        </xsl:for-each>
        motifs.length = <xsl:value-of select="count(/mast/motifs/motif)"/>;
        var SEQ_MAX = <xsl:value-of select="$max_seq_len"/>;
        var TRANSLATE_DNA = &quot;<xsl:value-of select="/mast/model/translate_dna/@value"/>&quot;;
        var DATABASE_TYPE = &quot;<xsl:value-of select="/mast/sequences/database/@type"/>&quot;;
        var STRAND_HANDLING = &quot;<xsl:value-of select="/mast/model/strand_handling/@value"/>&quot;;
        <xsl:call-template name="mast-to-html.js"/>
      </script>
  </xsl:template>

  <xsl:template name="html-body">
    <body onload="javascript:setup()" >
      <xsl:call-template name="help-popups"/>
      <xsl:call-template name="top"/>
      <xsl:call-template name="navigation"/>
      <xsl:call-template name="inputs"/>
      <xsl:call-template name="results"/>
      <xsl:call-template name="program"/>
      <xsl:call-template name="documentation"/>
      <xsl:call-template name="data"/>
      <xsl:call-template name="footer"/>
      <span class="sequence" id="ruler" style="visibility:hidden; white-space:nowrap;">ACGTN</span>
    </body>
  </xsl:template>

  <xsl:template name="help-popups">
    <div class="pop_content" id="pop_db_name">
      <p>The name of the sequence database file.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_db_seqs">
      <p>The number of sequences in the database.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_db_residues">
      <p>The number of residues in the sequence database.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_db_lastmod">
      <p>The date of the last modification to the sequence database.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_id">
      <p>The name of the motif. If the motif has been removed or removal is 
        recommended to avoid highly similar motifs then it will be displayed
        in red text.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_width">
      <p>The width of the motif. No gaps are allowed in motifs supplied to MAST
        as it only works for motifs of a fixed width.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_bmatch">
      <p>The sequence that would achieve the best possible match score and its
        reverse complement for nucleotide motifs.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_similarity">
      <p>MAST computes the pairwise correlations between each pair of motifs. 
        The correlation between two motifs is the maximum sum of Pearson's 
        correlation coefficients for aligned columns divided by the width of 
        the shorter motif. The maximum is found by trying all alignments of the
        two motifs. Motifs with correlations below 0.60 have little effect on
        the accuracy of the combined scores. Pairs of motifs with higher
        correlations should be removed from the query. Correlations above the
        supplied threshold are shown in red text. </p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_nos">
      <p>This diagram shows the normal spacing of the motifs specified to MAST.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_sequence">
      <p>The name of the sequence. This maybe be linked to search a sequence database for the sequence name.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_evalue">
      <p>
        The <a href="#evalue_doc"><i>E</i>-value</a> of the sequence. For DNA
        only; if strands were scored seperately then there will be two 
        <i>E</i>-values for the sequence seperated by a "/". The score for the
        provided sequence will be first and the score for the reverse-complement
        will be second.
      </p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_diagram" style="width:500px;">
      <p>The block diagram shows the best non-overlapping tiling of motif matches on the sequence.</p>
      <ul style="padding-left: 1em; margin-top:5px;">
        <li>The length of the line shows the length of a sequence relative to all the other sequences.</li>
        <li>A block is shown where the <a href="#pos_pvalue_doc" >positional <i>p</i>-value</a>
          of a motif is less (more significant) than the significance threshold which is 0.0001 by default.</li>
        <li>If a significant motif match (as specified above) overlaps other significant motif matches then 
          it is only displayed as a block if its <a href="#pos_pvalue_doc" >positional <i>p</i>-value</a> 
          is less (more significant) then the product of the <a href="#pos_pvalue_doc" >positional 
          <i>p</i>-values</a> of the significant matches that it overlaps.</li>
        <li>The position of a block shows where a motif has matched the sequence.</li>
        <li>The width of a block shows the width of the motif relative to the length of the sequence.</li>
        <li>The colour and border of a block identifies the matching motif as in the legend.</li>
        <li>The height of a block gives an indication of the significance of the match as 
          taller blocks are more significant. The height is calculated to be proportional 
          to the negative logarithm of the <a href="#pos_pvalue_doc" >positional <i>p</i>-value</a>, 
          truncated at the height for a <i>p</i>-value of 1e-10.</li>
          <li>Hovering the mouse cursor over the block causes the display of the motif name 
            and other details in the hovering text.</li>
        <li>DNA only; blocks displayed above the line are a match on the given DNA, whereas blocks 
          displayed below the line are matches to the reverse-complement of the given DNA.</li>
        <li>DNA only; when strands are scored separately then blocks may overlap on opposing strands.</li>
      </ul>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_sequence_description">
      <p>The description appearing after the identifier in the fasta file used to specify the sequence.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_combined_pvalue">
      <p>The <a href="#combined_pvalue_doc">combined <i>p</i>-value</a> of the
        sequence. DNA only; if strands were scored seperately then there will be
        two <i>p</i>-values for the sequence seperated by a "/". The score for
        the provided sequence will be first and the score for the
        reverse-complement will be second.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_frame">
      <p>This indicates the offset used for translation of the DNA.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_annotated_sequence">
      <p>The annotated sequence shows a portion of the sequence with the
        matching motif sequences displayed above. The displayed portion of the
        sequence can be modified by sliding the two buttons below the sequence
        block diagram so that the portion you want to see is between the two
        needles attached to the buttons. By default the two buttons move
        together but you can drag one individually by holding shift before you
        start the drag. If the strands were scored seperately then they can't
        be both displayed at once due to overlaps and so a radio button offers
        the choice of strand to display.</p>
      <div class="pop_close">[<a href="javascript:help_popup()">close</a> ]</div>
    </div>
  </xsl:template>

  <xsl:template name="top">
    <div id="top" class="pad1">
      <div class="prog_logo big">
        <img src="{$site_url}/doc/images/mast_icon.png" alt="MAST Logo"/>
        <h1>MAST</h1>
        <h2>Motif Alignment &amp; Search Tool</h2>
      </div>
      <p class="spaced">
        For further information on how to interpret these results or to get a 
        copy of the MEME software please access 
        <a href="http://meme.nbcr.net/">http://meme.nbcr.net</a>. 
      </p>
      <p>If you use MAST in your research, please cite the following paper:<br />
        <span class="citation">
          Timothy L. Bailey and Michael Gribskov,
          &quot;Combining evidence using p-values: application to sequence homology searches&quot;,
          <i>Bioinformatics</i>, <b>14</b>(1):48-54, 1998.
          <a href="http://bioinformatics.oxfordjournals.org/content/14/1/48">[pdf]</a>
        </span>
      </p>
    </div>
  </xsl:template>

  <xsl:template name="navigation">
    <div id="navigation" class="pad2">
      <a class="jump" href="#inputs">Inputs</a>
      &nbsp;&nbsp;|&nbsp;&nbsp;
      <a class="jump" href="#results">Search Results</a>
      &nbsp;&nbsp;|&nbsp;&nbsp;
      <a class="jump" href="#program">Program information</a>
      &nbsp;&nbsp;|&nbsp;&nbsp;
      <a class="jump" href="#doc">Explanation</a>
    </div>    
  </xsl:template>

  <xsl:template name="header">
    <xsl:param name="title" />
    <xsl:param name="self" select="$title" />
    <xsl:param name="prev" select="''" />
    <xsl:param name="next" select="''" />

    <table id="{$self}" width="100%" border="0" cellspacing="1" cellpadding="4" bgcolor="#FFFFFF">
      <tr>
        <td>
          <h2 class="mainh"><xsl:value-of select="$title"/></h2>
        </td>
        <td align="right" valign="bottom">
          <xsl:if test="$prev != ''"><a href="#{$prev}">Previous</a>&nbsp;</xsl:if>
          <xsl:if test="$next != ''"><a href="#{$next}">Next</a>&nbsp;</xsl:if>
          <a href="#top">Top</a>
        </td>
      </tr>
    </table>
  </xsl:template>

  <xsl:template name="inputs" >
    <xsl:call-template name="header">
      <xsl:with-param name="title" select="'Inputs'"/>
      <xsl:with-param name="self" select="'inputs'"/>
    </xsl:call-template>
    <div class="box">
      <h4 id="databases">Sequence Databases</h4>
      <div class="pad">
        <p>
          <xsl:text>The following sequence database</xsl:text>
          <xsl:choose>
            <xsl:when test="count(/mast/sequences/database) &gt; 1">
              <xsl:text>s were</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text> was</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:text> supplied to MAST.</xsl:text>
        </p>
        <table class="padded-table" border="0" >
          <col />
          <col />
          <col />
          <thead>
            <tr>
              <th style="text-align:left;" >Database <div class="help" onclick="help_popup(this,'pop_db_name')"></div></th>
              <th>Sequence Count <div class="help" onclick="help_popup(this,'pop_db_seqs')"></div></th>
              <th>Residue Count <div class="help" onclick="help_popup(this,'pop_db_residues')"></div></th>
              <th>Last Modified <div class="help" onclick="help_popup(this,'pop_db_lastmod')"></div></th>
            </tr>
          </thead>
          <tbody>
            <xsl:for-each select="/mast/sequences/database">
              <tr>
                <td><xsl:value-of select="@name"/></td>
                <td class="tnum"><xsl:value-of select="@seq_count"/></td>
                <td class="tnum"><xsl:value-of select="@residue_count"/></td>
                <td><xsl:value-of select="@last_mod_date"/></td>
              </tr>
            </xsl:for-each>
          </tbody>
          <tfoot>
            <tr>
              <th style="text-align:left; padding:5px 10px;">Total</th>
              <td class="tnum"><xsl:value-of select="sum(/mast/sequences/database/@seq_count)"/></td>
              <td class="tnum"><xsl:value-of select="sum(/mast/sequences/database/@residue_count)"/></td>
              <td>&nbsp;</td>
            </tr>
          </tfoot>
        </table>
      </div>
      <h4 id="motifs">Motifs</h4>
      <div class="pad">
        <p>The following motifs were supplied to MAST from &quot;<xsl:value-of select="/mast/motifs/@name" />&quot; last modified on <xsl:value-of 
            select="/mast/motifs/@last_mod_date"/>.
          <xsl:if test="count(/mast/motifs/motif[@bad = 'y']) &gt; 0">
            <xsl:choose>
              <xsl:when test="/mast/model/remove_correlated/@value = 'y'">
                Motifs with a red name have been removed because they have a similarity greater 
                than <xsl:value-of select="format-number(/mast/model/max_correlation, '0.00')" /> with another motif.
              </xsl:when>
              <xsl:otherwise>
                It is recommended that motifs with a red name be removed because they have a similarity greater 
                than <xsl:value-of select="format-number(/mast/model/max_correlation, '0.00')" /> with another motif.
              </xsl:otherwise>
            </xsl:choose>
          </xsl:if>
        </p>
        <table class="padded-table" border="0" >
          <col />
          <xsl:choose>
            <xsl:when test="/mast/alphabet/@type = 'nucleotide'">
              <col />
              <col />
            </xsl:when>
            <xsl:otherwise>
              <col />
            </xsl:otherwise>
          </xsl:choose>
          <col />
          <xsl:for-each select="/mast/motifs/motif">
            <col />
          </xsl:for-each>
          <thead>
            <tr>
              <th>&nbsp;</th>
              <th>&nbsp;</th>
              <xsl:choose>
                <xsl:when test="/mast/alphabet/@type = 'nucleotide'">
                  <th colspan="2">Best possible match <div class="help" onclick="help_popup(this,'pop_motif_bmatch')"></div></th>
                </xsl:when>
                <xsl:otherwise>
                  <th>&nbsp;</th>
                </xsl:otherwise>
              </xsl:choose>
              <th colspan="{count(/mast/motifs/motif)}">Similarity <div class="help" onclick="help_popup(this,'pop_motif_similarity')"></div></th>
            </tr>
            <tr>
              <th>Motif <div class="help" onclick="help_popup(this,'pop_motif_id')"></div></th>
              <th>Width <div class="help" onclick="help_popup(this,'pop_motif_width')"></div></th>
              <xsl:choose>
                <xsl:when test="/mast/alphabet/@type = 'nucleotide'">
                  <th>(+)</th>
                  <th>(-)</th>
                </xsl:when>
                <xsl:otherwise>
                  <th>Best possible match <div class="help" onclick="help_popup(this,'pop_motif_bmatch')"></div></th>
                </xsl:otherwise>
              </xsl:choose>
              <xsl:for-each select="/mast/motifs/motif">
                <th>
                  <xsl:if test="number(@name) = NaN">
                    <xsl:text>Motif </xsl:text>
                  </xsl:if>
                  <xsl:value-of select="@name" />
                </th>
              </xsl:for-each>
            </tr>
          </thead>
          <tbody>
            <xsl:for-each select="/mast/motifs/motif">
              <xsl:variable name="motif_a_num" select="position()" />
              <xsl:variable name="motif_a" select="@id" />
              <tr>
                <xsl:choose>
                  <xsl:when test="@bad = 'y'">
                    <td style="color:red;"><xsl:value-of select="@name" /></td>
                  </xsl:when>
                  <xsl:otherwise>
                    <td><xsl:value-of select="@name" /></td>
                  </xsl:otherwise>
                </xsl:choose>
                <td><xsl:value-of select="@width" /></td>
                <td class="sequence"><xsl:call-template name="colour-sequence" ><xsl:with-param name="seq" select="@best_f" /></xsl:call-template></td>
                <xsl:if test="/mast/alphabet/@type = 'nucleotide'">
                  <td class="sequence"><xsl:call-template name="colour-sequence" ><xsl:with-param name="seq" select="@best_r" /></xsl:call-template></td>
                </xsl:if>
                <xsl:for-each select="/mast/motifs/motif">
                  <xsl:variable name="motif_b_num" select="position()" />
                  <xsl:variable name="motif_b" select="@id" />
                  <xsl:choose>
                    <xsl:when test="$motif_a != $motif_b">
                      <xsl:variable name="correlation">
                        <xsl:choose>
                          <xsl:when test="/mast/motifs/correlation[@motif_a = $motif_a and @motif_b = $motif_b]">
                            <xsl:value-of select="/mast/motifs/correlation[@motif_a = $motif_a and @motif_b = $motif_b]/@value"/>
                          </xsl:when>
                          <xsl:otherwise>
                            <xsl:value-of select="/mast/motifs/correlation[@motif_a = $motif_b and @motif_b = $motif_a]/@value"/>
                          </xsl:otherwise>
                        </xsl:choose>
                      </xsl:variable>
                      <xsl:choose>
                        <xsl:when test="$correlation &gt;= /mast/model/max_correlation">
                          <td style="color:red;"><xsl:value-of select="$correlation" /></td>
                        </xsl:when>
                        <xsl:otherwise>
                          <td><xsl:value-of select="$correlation" /></td>
                        </xsl:otherwise>
                      </xsl:choose>
                    </xsl:when>
                    <xsl:otherwise>
                      <td>-</td>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:for-each>
              </tr>
            </xsl:for-each>
          </tbody>
        </table>
      </div>
      <xsl:if test="/mast/motifs/nos" >
        <xsl:variable name="base_size">
          <xsl:choose>
            <xsl:when test="/mast/model/translate_dna/@value = 'y'">3</xsl:when>
            <xsl:otherwise>1</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <h4 id="nos">Nominal Order and Spacing <div class="help" onclick="help_popup(this, 'pop_nos')"></div></h4>
        <div class="pad">
          <p>The expected order and spacing of the motifs.</p>
          <table style="width:100%;">
            <tr>
              <td class="block_td">
                <div class="block_container">
                  <div class="block_rule" style="width:{((/mast/motifs/nos/@length * $base_size) div $max_seq_len) * 100}%"></div>
                  <xsl:for-each select="/mast/motifs/nos/expect">
                    <xsl:variable name="motif" select="id(@motif)" />
                    <xsl:variable name="motif_num" select="count($motif/preceding-sibling::*) + 1" />

                    <xsl:call-template name="site">
                      <xsl:with-param name="diagram_len" select="$max_seq_len"/>
                      <xsl:with-param name="best_log_pvalue" select="$max_log_pvalue"/>
                      <xsl:with-param name="position" select="@pos * $base_size"/>
                      <xsl:with-param name="width" select="$motif/@width"/>
                      <xsl:with-param name="index" select="$motif_num"/>
                      <xsl:with-param name="name" select="$motif/@name"/>
                    </xsl:call-template>
                  </xsl:for-each>
                </div>
              </td>
            </tr>
            <tr>
              <td class="block_td" style="color: blue;">
                <div class="block_container" >
                  <xsl:call-template name="ruler">
                    <xsl:with-param name="max" select="/mast/motifs/nos/@length" />
                  </xsl:call-template>
                </div>
              </td>
            </tr>
          </table>
        </div>
      </xsl:if>
    </div>
  </xsl:template>

  <xsl:template name="results">

    <xsl:call-template name="header">
      <xsl:with-param name="title" select="'Search Results'"/>
      <xsl:with-param name="self" select="'results'"/>
    </xsl:call-template>
    <div class="box">
      <h4>Top Scoring Sequences</h4>
      <div class="pad">
        <p>
          Each of the following <xsl:value-of select="count(/mast/sequences/sequence)"/> sequences has an <i>E</i>-value less than 
          <xsl:value-of select="/mast/model/max_seq_evalue"/>.<br />
          The motif matches shown have a position p-value less than <xsl:value-of select="/mast/model/max_weak_pvalue"/>.<br />
          <xsl:if test="count(/mast/sequences/sequence[@known_pos = 'y'])" >
          The highlighted rows have had correct positions provided for them.<br />
          </xsl:if>
          <b>Click on the arrow</b> (&more;) next to the <i>E</i>-value to view more information about a sequence.
        </p>
        <xsl:call-template name="mast_legend"/>
        <table id="tbl_sequences" style="width:100%; table-layout:fixed;" border="0">
          <col style="width:{$seq_name_col_size}em;" />
          <xsl:choose>
            <xsl:when test="/mast/model/strand_handling/@value = 'separate'">
              <col style="width:5em;" />
              <col style="width:1em;" />
              <col style="width:5em;" />
            </xsl:when>
            <xsl:otherwise>
              <col style="width:6em;" />
            </xsl:otherwise>
          </xsl:choose>
          <col style="width:3em;" />
          <col />
          <thead>
            <th>Sequence <div class="help" onclick="help_popup(this,'pop_sequence')"></div></th>
          <xsl:choose>
            <xsl:when test="/mast/model/strand_handling/@value = 'separate'">
              <th colspan="3"><i>E</i>-value <div class="help" onclick="help_popup(this,'pop_evalue')"></div></th>
            </xsl:when>
            <xsl:otherwise>
              <th><i>E</i>-value <div class="help" onclick="help_popup(this,'pop_evalue')"></div></th>
            </xsl:otherwise>
          </xsl:choose>
            <th>&nbsp;</th>
            <th>Block Diagram <div class="help" onclick="help_popup(this,'pop_diagram')"></div></th>
          </thead>
          <tfoot>
            <tr>
              <xsl:choose>
                <xsl:when test="/mast/model/strand_handling/@value = 'separate'">
                  <td colspan="5">&nbsp;</td>
                </xsl:when>
                <xsl:otherwise>
                  <td colspan="3">&nbsp;</td>
                </xsl:otherwise>
              </xsl:choose>
              <td class="block_td" style="color: blue;">
                <div class="block_container" >
                  <xsl:call-template name="ruler">
                    <xsl:with-param name="max" select="$max_seq_len" />
                  </xsl:call-template>
                </div>
              </td>
            </tr>
          </tfoot>
          <tbody>
            <xsl:for-each select="/mast/sequences/sequence" >
              <xsl:variable name="db" select="id(@db)"/>
              <xsl:variable name="seq_is_dna" select="$db/@type = 'nucleotide'"/>
              <xsl:variable name="mul_width">
                <xsl:choose>
                  <xsl:when test="/mast/alphabet/@type = 'amino-acid' and $seq_is_dna">3</xsl:when>
                  <xsl:otherwise>1</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <xsl:variable name="seq_fract" select="(@length div $max_seq_len) * 100" />
              <xsl:variable name="highlight">
                <xsl:if test="@known_pos = 'y'">
                  <xsl:text>highlight</xsl:text>
                </xsl:if>
              </xsl:variable>
              <tr id="{@id}_blocks" class="{$highlight}">
                <td>
                <xsl:choose>
                  <xsl:when test="$db/@link">
                    <xsl:variable name="link">
                      <xsl:value-of select="substring-before($db/@link, 'SEQUENCEID')" />
                      <xsl:value-of select="@name" />
                      <xsl:value-of select="substring-after($db/@link, 'SEQUENCEID')" />
                    </xsl:variable>
                    <a href="{$link}"><xsl:value-of select="@name" /></a>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="@name" />
                  </xsl:otherwise>
                </xsl:choose>
                </td>
                <xsl:choose>
                  <xsl:when test="/mast/model/strand_handling/@value = 'norc'">
                    <td><xsl:value-of select="./score[@strand = 'forward']/@evalue" /></td>
                  </xsl:when>
                  <xsl:when test="/mast/model/strand_handling/@value = 'separate'">
                    <xsl:variable name="f_eval" select="./score[@strand = 'forward']/@evalue"/>
                    <xsl:variable name="r_eval" select="./score[@strand = 'reverse']/@evalue"/>
                    <xsl:variable name="f_dim"><xsl:if test="not($f_eval) or $r_eval and $f_eval &gt; $r_eval">dim</xsl:if></xsl:variable>
                    <xsl:variable name="r_dim"><xsl:if test="not($r_eval) or $f_eval and $r_eval &gt; $f_eval">dim</xsl:if></xsl:variable>
                    <td class="{$f_dim}">
                      <xsl:choose>
                        <xsl:when test="$f_eval" >
                          <xsl:value-of select="$f_eval" />
                        </xsl:when>
                        <xsl:otherwise><xsl:text>--</xsl:text></xsl:otherwise>
                      </xsl:choose>
                    </td>
                    <td>/</td>
                    <td class="{$r_dim}">
                      <xsl:choose>
                        <xsl:when test="$r_eval" >
                          <xsl:value-of select="$r_eval" />
                        </xsl:when>
                        <xsl:otherwise><xsl:text>--</xsl:text></xsl:otherwise>
                      </xsl:choose>
                    </td>
                  </xsl:when>
                  <xsl:otherwise>
                    <td><xsl:value-of select="./score/@evalue" /></td>
                  </xsl:otherwise>
                </xsl:choose>
                <td><a href="javascript:show_more('{@id}')" class="more_arrow" title="Toggle additional information">&more;</a></td>
                <td class="block_td">
                  <div class="block_container" id="{@id}_block_container" >
                    <xsl:if test="$seq_is_dna">
                      <div class="block_plus_sym" >+</div>
                      <div class="block_minus_sym" >-</div>
                    </xsl:if>
                    <div class="block_rule" style="width:{$seq_fract}%"></div>
                    <xsl:for-each select="seg/hit">
                      <xsl:variable name="motif" select="id(@motif)" />
                      <xsl:variable name="motif_num" select="count($motif/preceding-sibling::*) + 1" />
                      <xsl:variable name="strand">
                        <xsl:choose>
                          <xsl:when test="@strand = 'forward'"><xsl:text>plus</xsl:text></xsl:when>
                          <xsl:when test="@strand='reverse'"><xsl:text>minus</xsl:text></xsl:when>
                          <xsl:otherwise>
                          <xsl:value-of select="@strand" />
                          </xsl:otherwise>
                        </xsl:choose>
                      </xsl:variable>
                      <xsl:variable name="frame">
                        <xsl:if test="/mast/model/translate_dna/@value = 'y'">
                          <xsl:variable name="frame_index" select="(@pos - 1) mod 3"/>
                          <xsl:choose>
                            <xsl:when test="$frame_index = 0">
                              <xsl:text>a</xsl:text>
                            </xsl:when>
                            <xsl:when test="$frame_index = 1">
                              <xsl:text>b</xsl:text>
                            </xsl:when>
                            <xsl:otherwise>
                              <xsl:text>c</xsl:text>
                            </xsl:otherwise>
                          </xsl:choose>
                        </xsl:if>
                      </xsl:variable>
                      <xsl:call-template name="site">
                        <xsl:with-param name="diagram_len" select="$max_seq_len"/>
                        <xsl:with-param name="best_log_pvalue" select="$max_log_pvalue"/>
                        <xsl:with-param name="position" select="@pos"/>
                        <xsl:with-param name="width" select="$motif/@width * $mul_width"/>
                        <xsl:with-param name="index" select="$motif_num"/>
                        <xsl:with-param name="strand" select="$strand"/>
                        <xsl:with-param name="pvalue" select="@pvalue"/>
                        <xsl:with-param name="name" select="$motif/@name"/>
                        <xsl:with-param name="frame" select="$frame"/>
                      </xsl:call-template>
                    </xsl:for-each>
                  </div>
                </td>
              </tr>
            </xsl:for-each>
          </tbody>
        </table>
        <xsl:call-template name="mast_legend"/>
      </div>
    </div>
  </xsl:template>

  <xsl:template name="mast_legend">
    <div style="text-align:right">
      <xsl:for-each select="/mast/motifs/motif">
        <xsl:call-template name="legend_motif">
          <xsl:with-param name="name" select="@name"/>
          <xsl:with-param name="index" select="position()"/>
        </xsl:call-template>
      </xsl:for-each>
    </div>
  </xsl:template>

  <xsl:template name="program">
    <div id="program" class="bar">
      <div style="text-align:right;"><a href="#top">Top</a></div>
      <div class="subsection">
        <h5 id="version">MAST version</h5>
        <xsl:value-of select="/mast/@version"/> (Release date: <xsl:value-of select="/mast/@release"/>)
      </div>
      <div class="subsection">
        <h5 id="reference">Reference</h5>
        <span class="citation">
          Timothy L. Bailey and Michael Gribskov,
          &quot;Combining evidence using p-values: application to sequence homology searches&quot;,
          <i>Bioinformatics</i>, <b>14</b>(1):48-54, 1998.
        </span>
      </div>
      <div class="subsection">
        <h5 id="command">Command line summary</h5>
        <textarea rows="1" style="width:100%;" readonly="readonly">
          <xsl:value-of select="/mast/model/command_line"/>
        </textarea>
        <br />
        <xsl:if test="/mast/alphabet/@bg_source != 'sequence'">
          <xsl:text>Background letter frequencies (from </xsl:text>
          <xsl:choose>
            <xsl:when test="/mast/alphabet/@bg_source = 'preset'">
              <xsl:text>non-redundant database</xsl:text>
            </xsl:when>
            <xsl:when test="/mast/alphabet/@bg_source = 'file'">
              <xsl:value-of select="/mast/alphabet/@bg_file"/>
            </xsl:when>
          </xsl:choose>
          <xsl:text>):</xsl:text><br/>
          <div style="margin-left:25px;">
            <xsl:for-each select="/mast/alphabet/letter[not(@ambig) or @ambig = 'n']">
              <xsl:value-of select="@symbol"/><xsl:text>:&nbsp;</xsl:text><xsl:value-of select="format-number(@bg_value, '0.000')" />
              <xsl:if test="position() != last()"><xsl:text>&nbsp;&nbsp; </xsl:text></xsl:if>            
            </xsl:for-each>
          </div><br />
        </xsl:if>
        <xsl:if test="/mast/alphabet/@bg_source = 'sequence'">
          <xsl:text>Using background model based on each target's sequence composition.</xsl:text><br />
        </xsl:if>
        <xsl:text>Result calculation took </xsl:text><xsl:value-of select="/mast/runtime/@seconds"/><xsl:text> seconds</xsl:text>
        <br />
      </div>      
      <a href="javascript:showHidden('model')" id="model_activator">show model parameters...</a>
      <div class="subsection" id="model_data" style="display:none;">
        <h5>Model parameters</h5>
        <xsl:text>&newline;</xsl:text>
        <textarea style="width:100%;" rows="{count(/mast/model/*) - 1}" readonly="readonly">
          <xsl:variable name="spaces" select="'                    '"/>
          <xsl:text>&newline;</xsl:text>
          <xsl:for-each select="/mast/model/*[name(.) != 'command_line']">
            <xsl:variable name="pad" select="substring($spaces,string-length(name(.)))"/>
            <xsl:value-of select="name(.)"/>
            <xsl:value-of select="$pad"/>
            <xsl:text> = </xsl:text>
            <xsl:choose>
              <xsl:when test="count(@*) &gt; 0">
                <xsl:for-each select="@*">
                  <xsl:value-of select="name(.)"/>
                  <xsl:text>: "</xsl:text>
                  <xsl:value-of select="."/>
                  <xsl:text>"</xsl:text>
                  <xsl:if test="position() != last()">
                    <xsl:text>, </xsl:text>
                  </xsl:if>
                </xsl:for-each>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="normalize-space(.)"/>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:text>&newline;</xsl:text>
          </xsl:for-each>
        </textarea>
      </div>
      <a href="javascript:hideShown('model')" style="display:none;" id="model_deactivator">hide model parameters...</a>
    </div>
  </xsl:template>

  <xsl:template name="documentation">
    <span class="explain">
    <xsl:call-template name="header">
      <xsl:with-param name="title" select="'Explanation of MAST Results'"/>
      <xsl:with-param name="self" select="'doc'"/>
    </xsl:call-template>
      <div class="box">
        <h4>The MAST results consist of</h4>
        <ul>
          <li>The <a href="#input_motifs_doc"><b>inputs</b></a> to MAST including:
            <ol>
              <li>The <a href="#databases_doc"><b>sequence databases</b></a> showing the sequence 
                and residue counts. [<a href="#databases">View</a>]</li>
              <li>The <a href="#motifs_doc"><b>motifs</b></a> showing the name, width, best scoring match 
                and similarity to other motifs. [<a href="#motifs">View</a>]</li> 
              <li>
                The <a href="#nos_doc"><b>nominal order and spacing</b></a> diagram.<xsl:if test="/mast/motifs/nos"> [<a href="#nos">View</a>]</xsl:if>
              </li>
            </ol>
          </li>
          <li>The <a href="#sequences_doc"><b>search results</b></a> showing top scoring sequences with 
            tiling of all of the motifs matches shown for each of the sequences. [<a href="#results">View</a>]
          </li>
          <li>The <b>program</b> details including:
            <ol>
              <li>The <b>version</b> of MAST and the date it was released. [<a href="#version">View</a>]</li>
              <li>The <b>reference</b> to cite if you use MAST in your research. [<a href="#reference">View</a>]</li>
              <li>The <b>command line summary</b> detailing the parameters with which you ran MAST. [<a href="#command">View</a>]</li>
            </ol>
          </li>
          <li>This <b>explanation</b> of how to interpret MAST results.</li>
        </ul>
        <h4 id="input_motifs_doc">Inputs</h4>
        <p>MAST received the following inputs.</p>
        <h5 id="databases_doc">Sequence Databases</h5>
        <div class="doc">
          <p>This table summarises the sequence databases specified to MAST.</p>
          <dl>
            <dt>Database</dt>
            <dd>The name of the database file.</dd>
            <dt>Sequence Count</dt>
            <dd>The number of sequences in the database.</dd>
            <dt>Residue Count</dt>
            <dd>The number of residues in the database.</dd>
          </dl>
        </div>
        <h5 id="motifs_doc">Motifs</h5>
        <div class="doc">
          <p>Summary of the motifs specified to MAST.</p>
          <dl>
            <dt>Name</dt>
            <dd>The name of the motif. If the motif has been removed or removal is recommended to avoid highly similar motifs 
              then it will be displayed in red text.</dd>
            <dt>Width</dt>
            <dd>The width of the motif. No gaps are allowed in motifs supplied to MAST as it only works for motifs of a fixed width.</dd>
            <dt>Best possible match</dt>
            <dd>The sequence that would achieve the best possible match score and its reverse complement for nucleotide motifs.</dd>
            <dt>Similarity</dt>
            <dd>
              MAST computes the pairwise correlations between each pair of motifs. The correlation between two motifs is the 
              maximum sum of Pearson's correlation coefficients for aligned columns divided by the width of the shorter motif. 
              The maximum is found by trying all alignments of the two motifs. Motifs with correlations below 0.60 have little 
              effect on the accuracy of the combined scores. Pairs of motifs with higher correlations should be removed from 
              the query. Correlations above the supplied threshold are shown in red text.
            </dd>
          </dl>
        </div>
        <h5 id="nos_doc">Nominal Order and Spacing</h5>
        <div class="doc">
          <p>This diagram shows the normal spacing of the motifs specified to MAST.</p>
        </div>
        <h4 id="sequences_doc">Search Results</h4>
        <p>MAST provides the following motif search results.</p>
        <h5>Top Scoring Sequences</h5>
        <div class="doc">
          <p>
            This table summarises the top scoring sequences with a <a href="#evalue_doc">Sequence <i>E</i>-value</a>
            better than the threshold (default 10). The sequences are sorted by the <a href="#evalue_doc">Sequence 
            <i>E</i>-value</a> from most to least significant.
          </p>
          <dl>
            <dt>Sequence</dt>
            <dd>The name of the sequence. This maybe be linked to search a sequence database for the sequence name.</dd>
            <dt><i>E</i>-value</dt>
            <dd>
              The <a href="#evalue_doc"><i>E</i>-value</a> of the sequence. For DNA only; if strands were scored seperately 
              then there will be two <i>E</i>-values for the sequence seperated by a "/". The score for the provided sequence 
              will be first and the score for the reverse-complement will be second.
            </dd>
            <dt>&more;</dt>
            <dd>
              Click on this to show <a href="#additional_seq_doc">additional information</a> about the sequence such as a 
              description, combined p-value and the annotated sequence.
            </dd>
            <dt>Block Diagram</dt>
            <dd>
              The block diagram shows the best non-overlapping tiling of motif matches on the sequence.
              <ul style="padding-left: 1em; margin-top:5px;">
                <li>The length of the line shows the length of a sequence relative to all the other sequences.</li>
                <li>A block is shown where the <a href="#pos_pvalue_doc" >positional <i>p</i>-value</a>
                  of a motif is less (more significant) than the significance threshold which is 0.0001 by default.</li>
                <li>If a significant motif match (as specified above) overlaps other significant motif matches then 
                  it is only displayed as a block if its <a href="#pos_pvalue_doc" >positional <i>p</i>-value</a> 
                  is less (more significant) then the product of the <a href="#pos_pvalue_doc" >positional 
                  <i>p</i>-values</a> of the significant matches that it overlaps.</li>
                <li>The position of a block shows where a motif has matched the sequence.</li>
                <li>The width of a block shows the width of the motif relative to the length of the sequence.</li>
                <li>The colour and border of a block identifies the matching motif as in the legend.</li>
                <li>The height of a block gives an indication of the significance of the match as 
                  taller blocks are more significant. The height is calculated to be proportional 
                  to the negative logarithm of the <a href="#pos_pvalue_doc" >positional <i>p</i>-value</a>, 
                  truncated at the height for a <i>p</i>-value of 1e-10.</li>
                  <li>Hovering the mouse cursor over the block causes the display of the motif name 
                    and other details in the hovering text.</li>
                <li>DNA only; blocks displayed above the line are a match on the given DNA, whereas blocks 
                  displayed below the line are matches to the reverse-complement of the given DNA.</li>
                <li>DNA only; when strands are scored separately then blocks may overlap on opposing strands.</li>
              </ul>
            </dd>
          </dl>
        </div>
        <h5 id="additional_seq_doc">Additional Sequence Information</h5>
        <div class="doc">
          <p>
            Clicking on the &more; link expands a box below the sequence with additional information and adds two dragable buttons 
            below the block diagram.
          </p>
          <dl>
            <dt>Description</dt>
            <dd>The description appearing after the identifier in the fasta file used to specify the sequence.</dd>
            <dt>Combined <i>p</i>-value</dt>
            <dd>The <a href="#combined_pvalue_doc">combined <i>p</i>-value</a> of the sequence. DNA only; if strands were scored 
              seperately then there will be two <i>p</i>-values for the sequence seperated by a "/". The score for the provided sequence 
              will be first and the score for the reverse-complement will be second.</dd>
            <dt id="anno_doc">Annotated Sequence</dt>
            <dd>
              The annotated sequence shows a portion of the sequence with the matching motif sequences displayed above. 
              The displayed portion of the sequence can be modified by sliding the two buttons below the sequence block diagram
              so that the portion you want to see is between the two needles attached to the buttons. By default the two buttons
              move together but you can drag one individually by holding shift before you start the drag. If the strands were
              scored seperately then they can't be both displayed at once due to overlaps and so a radio button offers the choice
              of strand to display.
            </dd>
          </dl>
        </div>
        <h4>Scoring</h4>
        <p>MAST scores sequences using the following measures.</p>
        <h5 id="score_doc">Position score calculation</h5>
        <div class="doc">
          <p>
            The score for the match of a position in a sequence to a motif is computed by by summing the appropriate entry 
            from each column of the position-dependent scoring matrix that represents the motif. Sequences shorter than 
            one or more of the motifs are skipped.
          </p>
        </div>
        <h5 id="pos_pvalue_doc">Position <i>p</i>-value</h5>
        <div class="doc">
          <p>
            The position p-value of a match is the probability of a single random subsequence of the length of the motif 
            scoring at least as well as the observed match.
          </p>
        </div>
        <h5 id="seq_pvalue_doc">Sequence <i>p</i>-value</h5>
        <div class="doc">
          <p>
            The sequence p-value of a score is defined as the probability of a random sequence of the same length containing 
            some match with as good or better a score.
          </p>
        </div>
        <h5 id="combined_pvalue_doc">Combined <i>p</i>-value</h5>
        <div class="doc">
          <p>The combined p-value of a sequence measures the strength of the match of the sequence to all the motifs and is calculated by</p>
          <ol>
            <li>finding the score of the single best match of each motif to the sequence (best matches may overlap),</li>
            <li>calculating the sequence p-value of each score,</li>
            <li>forming the product of the p-values,</li>
            <li>taking the p-value of the product.</li>
          </ol>
        </div>
        <h5 id="evalue_doc">Sequence <i>E</i>-value</h5>
        <div class="doc">
          <p>
            The E-value of a sequence is the expected number of sequences in a random database of the same size that would match 
            the motifs as well as the sequence does and is equal to the combined p-value of the sequence times the number of 
            sequences in the database.
          </p>
        </div>
      </div>
    </span>
  </xsl:template>

  <xsl:template name="data" >
    <xsl:text>&newline;</xsl:text>
    <xsl:comment >This data is used by the javascript to create the detailed views</xsl:comment><xsl:text>&newline;</xsl:text>
    <form>
      <xsl:for-each select="/mast/sequences/sequence">
        <xsl:variable name="segs">
          <xsl:text>&newline;</xsl:text>
          <xsl:for-each select="seg"> 
            <xsl:value-of select="@start"/><xsl:text>&tab;</xsl:text>
            <xsl:value-of select="translate(data, ' &#10;&#9;', '')" />
            <xsl:text>&newline;</xsl:text>
          </xsl:for-each>
        </xsl:variable>
        <xsl:variable name="hits">
          <xsl:text>&newline;</xsl:text>
          <xsl:for-each select=".//hit"> 
            <xsl:value-of select="@pos" /><xsl:text>&tab;</xsl:text> 
            <xsl:value-of select="@motif" /><xsl:text>&tab;</xsl:text>
            <xsl:choose>
              <xsl:when test="not (@strand) or @strand = 'forward'">
                <xsl:text>+</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:text>-</xsl:text>
              </xsl:otherwise>
            </xsl:choose><xsl:text>&tab;</xsl:text> 
            <xsl:value-of select="@pvalue"/><xsl:text>&tab;</xsl:text> 
            <xsl:value-of select="@match"/><xsl:text>&tab;</xsl:text>
            <xsl:value-of select="@translation"/><xsl:text>&newline;</xsl:text> 
          </xsl:for-each>
        </xsl:variable>
        <xsl:variable name="combined_pvalue">
          <xsl:choose>
            <xsl:when test="/mast/model/strand_handling/@value = 'norc'">
              <xsl:value-of select="./score[@strand = 'forward']/@combined_pvalue" />
            </xsl:when>
            <xsl:when test="/mast/model/strand_handling/@value = 'separate'">
              <xsl:choose>
                <xsl:when test="./score[@strand = 'forward']/@combined_pvalue">
                  <xsl:value-of select="./score[@strand = 'forward']/@combined_pvalue" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:text>--&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</xsl:text>
                </xsl:otherwise>
              </xsl:choose>
              <xsl:text>&nbsp;&nbsp;/&nbsp;&nbsp;</xsl:text>
              <xsl:choose>
                <xsl:when test="./score[@strand = 'reverse']/@combined_pvalue">
                  <xsl:value-of select="./score[@strand = 'reverse']/@combined_pvalue" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:text>--</xsl:text>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="./score/@combined_pvalue" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:if test="/mast/model/translate_dna/@value = 'y'">
          <xsl:variable name="best_frame">
            <xsl:choose>
              <xsl:when test="/mast/model/strand_handling/@value = 'norc'">
                <xsl:value-of select="./score[@strand = 'forward']/@frame" />
              </xsl:when>
              <xsl:when test="/mast/model/strand_handling/@value = 'separate'">
                <xsl:choose>
                  <xsl:when test="./score[@strand = 'forward']/@frame">
                    <xsl:value-of select="./score[@strand = 'forward']/@frame" />
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:text>--&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</xsl:text>
                  </xsl:otherwise>
                </xsl:choose>
                <xsl:text>&nbsp;&nbsp;/&nbsp;&nbsp;</xsl:text>
                <xsl:choose>
                  <xsl:when test="./score[@strand = 'reverse']/@frame">
                    <xsl:value-of select="./score[@strand = 'reverse']/@frame" />
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:text>--</xsl:text>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="./score/@frame" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <input type="hidden" id="{@id}_frame" value="{$best_frame}" /><xsl:text>&newline;</xsl:text>
        </xsl:if>
        <input type="hidden" id="{@id}_len" value="{@length}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@id}_desc" value="{@comment}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@id}_combined_pvalue" value="{$combined_pvalue}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@id}_type" value="{id(@db)/@type}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@id}_segs" value="{$segs}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@id}_hits" value="{$hits}" /><xsl:text>&newline;</xsl:text>
      </xsl:for-each>
    </form>
  </xsl:template>

  <xsl:template name="colour-sequence">
    <xsl:param name="seq"/>
    <xsl:variable name="colour">
      <xsl:call-template name="pick-colour">
        <xsl:with-param name="sym" select="substring($seq, 1, 1)"/>
      </xsl:call-template>
    </xsl:variable>
    <span style="color:{$colour}"><xsl:value-of select="substring($seq, 1, 1)"/></span>
    <xsl:if test="string-length($seq) &gt; 1">
      <xsl:call-template name="colour-sequence">
        <xsl:with-param name="seq" select="substring($seq, 2)"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="pick-colour">
    <xsl:param name="sym"/>
    <xsl:param name="isnuc" select="/mast/alphabet/@type = 'nucleotide'" />
    <xsl:choose>
      <xsl:when test="$isnuc">
        <xsl:choose>
          <xsl:when test="$sym = 'A'">
            <xsl:text>red</xsl:text>
          </xsl:when>
          <xsl:when test="$sym = 'C'">
            <xsl:text>blue</xsl:text>
          </xsl:when>
          <xsl:when test="$sym = 'G'">
            <xsl:text>orange</xsl:text>
          </xsl:when>
          <xsl:when test="$sym = 'T'">
            <xsl:text>green</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>black</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="contains('ACFILVWM', $sym)">
            <xsl:text>blue</xsl:text>
          </xsl:when>
          <xsl:when test="contains('NQST', $sym)">
            <xsl:text>green</xsl:text>
          </xsl:when>
          <xsl:when test="contains('DE', $sym)">
            <xsl:text>magenta</xsl:text>
          </xsl:when>
          <xsl:when test="contains('KR', $sym)">
            <xsl:text>red</xsl:text>
          </xsl:when>
          <xsl:when test="$sym = 'H'">
            <xsl:text>pink</xsl:text>
          </xsl:when>
          <xsl:when test="$sym = 'G'">
            <xsl:text>orange</xsl:text>
          </xsl:when>
          <xsl:when test="$sym = 'P'">
            <xsl:text>yellow</xsl:text>
          </xsl:when>
          <xsl:when test="$sym = 'Y'">
            <xsl:text>turquoise</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>black</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="footer">
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
  </xsl:template>
</xsl:stylesheet>
