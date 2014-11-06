<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [
<!ENTITY nbsp "&#160;">
<!ENTITY space " ">
<!ENTITY nl "&#10;">
<!ENTITY tab "&#9;">
<!ENTITY more "&#8615;">
<!ENTITY less "&#8613;">
<!ENTITY upload "&#8674;">
<!ENTITY download "&#10225;">
<!ENTITY previous "&#8679;">
<!ENTITY next "&#8681;">
<!ENTITY asc "&#9660;">
<!ENTITY desc "&#9650;">
<!ENTITY open "&#9660;">
<!ENTITY closed "&#9654;">
<!ENTITY ellipsis "&#8230;">
]><!-- define nbsp as it is not defined in xml, only lt, gt and amp are defined by default -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="html" indent="yes" 
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/html4/loose.dtd"
    />

  <!-- contains constant site_url -->
  <xsl:include href="constants.xsl"/>
  <!-- css includes -->
  <xsl:include href="meme.css.xsl"/>
  <xsl:include href="dreme-to-html.css.xsl"/>
  <!-- javascript includes -->
  <xsl:include href="delay_draw.js.xsl"/>
  <xsl:include href="motif_logo.js.xsl"/>
  <xsl:include href="dreme-to-html.js.xsl"/>

  <!-- Stylesheet processing starts here -->
  <xsl:template match="/dreme">
    <html>
      <xsl:call-template name="html-head"/>
      <xsl:call-template name="html-body"/>
    </html>
  </xsl:template>

  <xsl:template name="html-head">
    <head>
      <title>DREME</title>
      <xsl:call-template name="html-css"/>
      <xsl:call-template name="html-js"/>
    </head>
  </xsl:template>

  <xsl:template name="html-css">
    <style type="text/css">
      <xsl:call-template name="meme.css" />
      <xsl:call-template name="dreme-to-html.css" />
    </style>
  </xsl:template>

  <xsl:template name="html-js">
    <script type="text/javascript">
      <xsl:call-template name="motif-info" />
      <xsl:call-template name="delay_draw.js" />
      <xsl:call-template name="motif_logo.js" />
      <xsl:call-template name="dreme-to-html.js" />
    </script>
  </xsl:template>

  <xsl:template name="motif-info">
    <xsl:text>&nl;      var pos_count = </xsl:text>
    <xsl:value-of select="/dreme/model/positives/@count"/>
    <xsl:text>;&nl;</xsl:text>
    <xsl:text>      var neg_count = </xsl:text>
    <xsl:value-of select="/dreme/model/negatives/@count"/>
    <xsl:text>;&nl;</xsl:text>
    <xsl:text>      var motif_seqs = [];&nl;</xsl:text>
    <xsl:for-each select="/dreme/motifs/motif">
      <xsl:text>      motif_seqs[</xsl:text>
      <xsl:value-of select="position()"/>
      <xsl:text>] = [&quot;</xsl:text>
      <xsl:value-of select="@id"/>
      <xsl:text>&quot;, &quot;</xsl:text>
      <xsl:value-of select="@seq"/>
      <xsl:text>&quot;, &quot;</xsl:text>
      <xsl:call-template name="util-rc">
        <xsl:with-param name="input" select="@seq"/>
      </xsl:call-template>
      <xsl:text>&quot;, </xsl:text>
      <xsl:value-of select="@length"/>
      <xsl:text>, </xsl:text>
      <xsl:value-of select="@nsites"/>
      <xsl:text>, </xsl:text>
      <xsl:value-of select="@p"/>
      <xsl:text>, </xsl:text>
      <xsl:value-of select="@n"/>
      <xsl:text>, </xsl:text>
      <xsl:value-of select="@pvalue"/>
      <xsl:text>, </xsl:text>
      <xsl:value-of select="@evalue"/>
      <xsl:text>, </xsl:text>
      <xsl:value-of select="@unerased_evalue"/>
      <xsl:text>, [</xsl:text>
      <xsl:for-each select="match">
        <xsl:sort select="@evalue" data-type="number"/>
        <xsl:if test="position() != 1"><xsl:text>, </xsl:text></xsl:if>
        <xsl:text>[&quot;</xsl:text>
        <xsl:value-of select="@seq"/>
        <xsl:text>&quot;, </xsl:text>
        <xsl:value-of select="@p"/>
        <xsl:text>, </xsl:text>
        <xsl:value-of select="@n"/>
        <xsl:text>, </xsl:text>
        <xsl:value-of select="@pvalue"/>
        <xsl:text>, </xsl:text>
        <xsl:value-of select="@evalue"/>
        <xsl:text>]</xsl:text>
      </xsl:for-each>
      <xsl:text>]];&nl;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="html-body">
    <body onload="page_loaded()" onpageshow="page_shown(event)" onresize="update_scrollpad()">
      <xsl:call-template name="hidden-data"/>
      <xsl:call-template name="help-popups"/>
      <xsl:call-template name="expanded-motif"/>
      <xsl:call-template name="grey-out-page"/>
      <xsl:call-template name="submit-to"/>
      <xsl:call-template name="download"/>
      <xsl:call-template name="top"/>
      <xsl:call-template name="description"/>
      <xsl:call-template name="motifs"/>
      <xsl:call-template name="program"/>
      <div id="scrollpad"></div>
    </body>
  </xsl:template>



  <xsl:template name="hidden-data">
    <xsl:variable name="bg" select="/dreme/model/background"/>
    <!-- alphabet -->
    <xsl:variable name="alphabet">
      <xsl:choose>
        <xsl:when test="$bg/@type = 'dna'">
          <xsl:text>ACGT</xsl:text>
        </xsl:when>
        <xsl:when test="$bg/@type = 'rna'">
          <xsl:text>ACGU</xsl:text>
        </xsl:when>
        <xsl:when test="$bg/@type = 'aa'">
          <xsl:text>ACDEFGHIKLMNPQRSTVWY</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <!-- background source -->
    <xsl:variable name="bgsrc">
      <xsl:choose>
        <xsl:when test="/dreme/model/background/@from = 'dataset'">
          <xsl:text>dataset</xsl:text>
        </xsl:when>
        <xsl:when test="/dreme/model/background/@from = 'file'">
          <xsl:value-of select="/dreme/model/background/@file"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <!-- background frequencies -->
    <xsl:variable name="bgfreq">
      <xsl:choose>
        <xsl:when test="$bg/@type = 'dna'">
          <!-- ACGT -->
          <xsl:value-of select="concat('A ', $bg/@A, ' C ', $bg/@C, 
            ' G ', $bg/@G, ' T ', $bg/@T)"/>
        </xsl:when>
        <xsl:when test="$bg/@type = 'rna'">
          <!-- ACGU -->
          <xsl:value-of select="concat('A ', $bg/@A, ' C ', $bg/@C, 
            ' G ', $bg/@G, ' U ', $bg/@U)"/>
        </xsl:when>
        <xsl:when test="$bg/@type = 'aa'">
          <!-- ACDEFGHIKLMNPQRSTVWY -->
          <xsl:value-of select="concat('A ', $bg/@A, ' C ', $bg/@C,
            ' D ', $bg/@D, ' E ', $bg/@E, ' F ', $bg/@F, ' G ', $bg/@G,
            ' H ', $bg/@H, ' I ', $bg/@I, ' K ', $bg/@K, ' L ', $bg/@L,
            ' M ', $bg/@M, ' N ', $bg/@N, ' P ', $bg/@P, ' Q ', $bg/@Q,
            ' R ', $bg/@R, ' S ', $bg/@S, ' T ', $bg/@T, ' V ', $bg/@V,
            ' W ', $bg/@W, ' Y ', $bg/@Y)"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <form id="submit_form" method="post" action="{$site_url}/cgi-bin/meme_request.cgi" target="_blank">
    <xsl:comment>+++++++++++++++START DATA+++++++++++++++</xsl:comment>
    <xsl:text>&nl;</xsl:text>
    <!-- the version is the first hidden input on the page parsed by meme-io -->
    <input type="hidden" name="version" value="MEME version {/dreme/@version}"/>
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" name="alphabet" id="alphabet" value="{$alphabet}"/>
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" name="strands" value="+ -"/>
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" name="bgsrc" value="{$bgsrc}"/>
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" name="bgfreq" id="bgfreq" value="{$bgfreq}"/>
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" name="name" value="{/dreme/model/positives/@name}"/>
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" name="nmotifs" id="nmotifs" value="{count(/dreme/motifs/motif)}"/>
    <xsl:text>&nl;</xsl:text>
    <xsl:for-each select="/dreme/motifs/motif">
      <xsl:comment>data for motif <xsl:value-of select="position()"/></xsl:comment>
      <xsl:text>&nl;</xsl:text>
      <xsl:call-template name="motif-data"/>
    </xsl:for-each>
    <xsl:comment>+++++++++++++++FINISHED DATA++++++++++++</xsl:comment>
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" name="program" id="submit_program" value="TOMTOM"/>
    <input type="hidden" name="motif" id="submit_motif" value="all"/>
    <input type="hidden" name="logoformat" id="submit_format" value="png"/>
    <input type="hidden" name="logorc" id="submit_rc" value="false"/>
    <input type="hidden" name="logossc" id="submit_ssc" value="false"/>
    <input type="hidden" name="logowidth" id="submit_width" value=""/>
    <input type="hidden" name="logoheight" id="submit_height" value="7.5"/>
    </form>
  </xsl:template>

  <xsl:template name="motif-data">
    <xsl:variable name="motif_name">
      <xsl:choose>
        <xsl:when test="@name">
          <xsl:value-of select="@name"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="@seq"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="pspm">
      <xsl:text>&nl;</xsl:text>
      <xsl:text>letter-probability matrix: alength= 4</xsl:text>
      <xsl:text> w= </xsl:text><xsl:value-of select="@length"/>
      <xsl:text> nsites= </xsl:text><xsl:value-of select="@nsites"/>
      <xsl:text> E= </xsl:text><xsl:value-of select="@evalue"/>
      <xsl:text>&nl;</xsl:text>
      <xsl:for-each select="pos">
        <xsl:value-of select="concat(@A, ' ', @C, ' ', @G, ' ', @T)"/>
        <xsl:text>&nl;</xsl:text>
      </xsl:for-each>
    </xsl:variable>
    <!-- finish constructing pspm value -->
    <input type="hidden" id="motifblock{position()}" 
      name="motifname{position()}" value="{$motif_name}" />
    <xsl:text>&nl;</xsl:text>
    <input type="hidden" id="pspm{position()}" 
      name="pspm{position()}" value="{$pspm}" />
    <xsl:text>&nl;</xsl:text>
  </xsl:template>

  <xsl:template name="help-popups">
    <div class="pop_content" id="pop_motifs_name">
      <p>
        The name of the motif uses the IUPAC codes for nucleotides which has 
        a different letter to represent each of the 15 possible combinations.
      </p>
      <p>
        The name is itself a representation of the motif though the position
        weight matrix is not directly equalivant as it is generated from the
        sites found that matched the letters given in the name.
      </p>
      <p>
        <a href="http://meme.nbcr.net/meme/doc/alphabets.html">
        Read more about the MEME suite's use of the IUPAC alphabets.
        </a>
      </p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motifs_logo">
      <p>The logo of the motif.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motifs_rc_logo">
      <p>The logo of the reverse complement motif.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motifs_evalue">
      <p>The E-value is the enrichment p-value times the number of candidate 
        motifs tested.</p>
      <p>The enrichment p-value is calculated using the Fisher Exact Test for 
        enrichment of the motif in the positive sequences.</p>
      <p>Note that the counts used in the Fisher Exact Test are made after 
        erasing sites that match previously found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motifs_uevalue">
      <p>The E-value of the motif calculated without erasing the sites of 
        previously found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_more">
      <p>Show more information on the motif.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_submit">
      <p>Submit your motif to another MEME Suite program.</p>
      <h5>Supported Programs</h5>
      <dl>
        <dt>Tomtom</dt>
        <dd>Tomtom is a tool for searching for similar known motifs. 
          [<a href="http://meme.nbcr.net/meme/tomtom-intro.html">manual</a>]</dd>
        <dt>MAST</dt>
        <dd>MAST is a tool for searching biological sequence databases for 
          sequences that contain one or more of a group of known motifs.
          [<a href="http://meme.nbcr.net/meme/mast-intro.html">manual</a>]</dd>
        <dt>FIMO</dt>
        <dd>FIMO is a tool for searching biological sequence databases for 
          sequences that contain one or more known motifs.
          [<a href="http://meme.nbcr.net/meme/fimo-intro.html">manual</a>]</dd>
        <dt>GOMO</dt>
        <dd>GOMO is a tool for identifying possible roles (Gene Ontology 
          terms) for DNA binding motifs.
          [<a href="http://meme.nbcr.net/meme/gomo-intro.html">manual</a>]</dd>
        <dt>SpaMo</dt>
        <dd>SpaMo is a tool for inferring possible transcription factor
          complexes by finding motifs with enriched spacings.
          [<a href="http://meme.nbcr.net/meme/spamo-intro.html">manual</a>]</dd>
      </dl>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_download">
      <p>Download your motif as a position weight matrix or a custom logo.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_positives">
      <p># positive sequences matching the motif / # positive sequences.</p>
      <p>Note these counts are made after erasing sites that match previously
        found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_negatives">
      <p># negative sequences matching the motif / # negative sequences.</p>
      <p>Note these counts are made after erasing sites that match previously
        found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_pvalue">
      <p>The p-value of the Fisher Exact Test for enrichment of the motif in 
        the positive sequences.</p>
      <p>Note that the counts used in the Fisher Exact Test are made after 
        erasing sites that match previously found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_evalue">
      <p>The E-value is the motif p-value times the number of candidate motifs 
        tested.</p>
      <p>Note that the p-value was calculated with counts made after 
        erasing sites that match previously found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_motif_uevalue">
      <p>The E-value of the motif calculated without erasing the sites of 
        previously found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_match_word">
      <p>All words matching the motif whose uncorrected p-value is less than
        <xsl:value-of select="/dreme/model/add_pv_thresh"/>.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_match_pos">
      <p># positive sequences with matches to the word / # positive sequences.</p>
      <p>Note these counts are made after erasing sites that match previously
        found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_match_neg">
      <p># negative sequences with matches to the word / # negative sequences.</p>
      <p>Note these counts are made after erasing sites that match previously
        found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_match_pval">
      <p>The p-value of the Fisher Exact Test for enrichment of the word in 
        the positive sequences.</p>
      <p>Note that the counts used in the Fisher Exact Test are made after 
        erasing sites that match previously found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
    <div class="pop_content" id="pop_match_eval">
      <p>The word p-value times the number of candidates tested.</p>
      <p>Note that the p-value was calculated with counts made after 
        erasing sites that match previously found motifs.</p>
      <div style="float:right; bottom:0px;">[<a href="javascript:help()">close</a> ]</div>
    </div>
  </xsl:template>

  <!-- When displayed hides the page behind a transparent div -->
  <xsl:template name="grey-out-page">
    <div id="grey_out_page" class="grey_background" style="display:none;">
    </div>
  </xsl:template>

  <xsl:template name="submit-to">
    <div class="popup_wrapper">
      <div id="send_to" class="popup" style="top:-150px; display:none">
        <div>
          <div style="float:left" id="send_to_title_1">
            <h2 class="mainh compact">Submit &quot;<span id="send_to_name"></span>&quot;</h2>
          </div>
          <div style="float:left" id="send_to_title_2">
            <h2 class="mainh compact">Submit All Motifs</h2>
          </div>
          <div style="float:right; ">
            <div class="close" onclick="both_hide();">x</div>
          </div>
          <div style="clear:both"></div>
        </div>
        <div style="padding:0 5px 5px 5px;">
          <div id="send_to_selector" style="height:100px;">
            <img id="send_to_img" height="90" style="max-width:170px;" onerror="fix_popup_logo(this, 'send_to_can', false)"/>
            <canvas id="send_to_can" width="10" height="100" style="display:none;"/>
            <img id="send_to_rcimg" height="90" style="max-width:170px;" onerror="fix_popup_logo(this, 'send_to_rccan', true);"/>
            <canvas id="send_to_rccan" width="10" height="100" style="display:none"/>
            <div style="float:right;">
              <a id="prev_arrow_2" href="javascript:both_change(-1)" class="navarrow">&previous;</a>
              <div id="pop_num_2" class="navnum"></div>
              <a id="next_arrow_2" href="javascript:both_change(1)" class="navarrow">&next;</a>
            </div>
          </div>
          <form>
            <div style="">
              <div style="float:left;">
                <h4 class="compact">Select what you want to do</h4>
                <div class="programs_scroll">
                  <ul id="tasks" class="programs">
                    <li id="search_motifs" onclick="click_submit_task(this)" class="selected">Search Motifs</li>
                    <li id="search_sequences" onclick="click_submit_task(this)">Search Sequences</li>
                    <li id="rank_sequences" onclick="click_submit_task(this)">Rank Sequences</li>
                    <li id="predict_go" onclick="click_submit_task(this)">Predict Gene Ontology terms</li>
                    <li id="infer_tf" onclick="click_submit_task(this)">Infer TF Complexes</li>
                  </ul>
                </div>
              </div>
              <div style="float:right;">
                <h4 class="compact">Select a program</h4>
                <div class="programs_scroll">
                  <ul id="programs" class="programs">
                    <li id="TOMTOM" onclick="click_submit_program(this)" class="selected">Tomtom</li>
                    <li id="FIMO" onclick="click_submit_program(this)">FIMO</li>
                    <li id="MAST" onclick="click_submit_program(this)">MAST</li>
                    <li id="GOMO" onclick="click_submit_program(this)">GOMO</li>
                    <li id="SPAMO" onclick="click_submit_program(this)">SpaMo</li>
                  </ul>
                </div>
              </div>
              <div style="font-weight:bold; display:inline-block; text-align:center; width:60px; height:100px; line-height:100px">Or</div>
              <div style="clear:both;"></div>
            </div>
            <h4><span id="program_action">Search Motifs</span> with <span id="program_name">Tomtom</span></h4>
            <p><span id="program_desc">Find similar motifs in published 
                libraries or a library you supply.</span></p>

            <div style="margin-top:10px; height: 2em;">
              <div style="float:left;">
                <input type="button" value="Send" onclick="javascript:send_to_submit()"/>
              </div>
              <div style="float:right;">
                <input type="button" value="Cancel" onclick="javascript:both_hide()"/>
              </div>
            </div>
          </form>
        </div>
      </div>
    </div>
  </xsl:template>

  <xsl:template name="download">
    <div class="popup_wrapper">
      <div id="download" class="popup" style="top:-150px; display:none">
        <div>
          <div style="float:left">
            <h2 class="mainh" style="margin:0; padding:0;">Download &quot;<span id="download_name"></span>&quot;</h2>
          </div>
          <div style="float:right; ">
            <div class="close" onclick="both_hide();">x</div>
          </div>
          <div style="clear:both"></div>
        </div>
        <div style="padding:0 5px 5px 5px;">
          <div style="height:100px">
            <img id="download_img" height="90" style="max-width:170px;" onerror="fix_popup_logo(this, 'download_can', false)"/>
            <canvas id="download_can" width="10" height="100" style="display:none;"/>
            <img id="download_rcimg" height="90" style="max-width:170px;" onerror="fix_popup_logo(this, 'download_rccan', true)"/>
            <canvas id="download_rccan" width="10" height="100" style="display:none;"/>
            <div style="float:right;">
              <a id="prev_arrow_1" href="javascript:both_change(-1)" class="navarrow">&previous;</a>
              <div id="pop_num_1" class="navnum"></div>
              <a id="next_arrow_1" href="javascript:both_change(1)" class="navarrow">&next;</a>
            </div>
          </div>
          <form>
            <input type="hidden" id="download_tab_num" value="1"/>
            <div style="padding:5px 0;">
              <div class="tabArea top">
                <a id="download_tab_1" 
                  href="javascript:click_download_tab(1)" 
                  class="tab activeTab">PSPM Format</a>
                <a id="download_tab_2" 
                  href="javascript:click_download_tab(2)" 
                  class="tab">PSSM Format</a>
                <a id="download_tab_3" 
                  href="javascript:click_download_tab(3)" 
                  class="tab">Logo</a>
              </div>
              <div class="tabMain">
                <div id="download_pnl_1">
                  <textarea id="download_pspm" style="width:99%" 
                    rows="10" readonly="readonly"></textarea>
                </div>
                <div id="download_pnl_2">
                  <textarea id="download_pssm" style="width:99%" 
                    rows="10" readonly="readonly"></textarea>
                </div>
                <div id="download_pnl_3">
                  <table>
                    <tr>
                      <td><label for="logo_format">Format:</label></td>
                      <td>
                        <select id="logo_format">
                          <option value="png">PNG (for web)</option>
                          <option value="eps">EPS (for publication)</option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td><label for="logo_rc">Orientation:</label></td>
                      <td>
                        <select id="logo_rc">
                          <option value="false">Normal</option>
                          <option value="true">Reverse Complement</option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td><label for="logo_ssc">Small Sample Correction:</label></td>
                      <td>
                        <select id="logo_ssc">
                          <option value="false">Off</option>
                          <option value="true">On</option>
                        </select>
                      </td>
                    </tr>
                    <tr>
                      <td><label for="logo_width">Width:</label></td>
                      <td>
                        <input type="text" id="logo_width" size="4"/>&nbsp;cm
                      </td>
                    </tr>
                    <tr>
                      <td><label for="logo_height">Height:</label></td>
                      <td>
                        <input type="text" id="logo_height" size="4"/>&nbsp;cm
                      </td>
                    </tr>
                  </table>
                </div>
                <div style="margin-top:10px;">
                  <div style="float:left;">
                    <input type="button" id="download_button" value="Download" 
                      style="visibility:hidden;" 
                      onclick="javascript:download_submit()"/>
                  </div>
                  <div style="float:right;">
                    <input type="button" value="Cancel" 
                      onclick="javascript:both_hide()"/>
                  </div>
                  <div style="clear:both;"></div>
                </div>
              </div>
            </div>
          </form>
        </div>
      </div>
    </div>
  </xsl:template>

  <xsl:template name="top">
    <a name="top"/>
    <div class="pad1">
      <h1><img src="{$site_url}/doc/images/dreme_logo.png" 
          alt="Discriminative Regular Expression Motif Elicitation (DREME)" /></h1>
      <p class="spaced">
        For further information on how to interpret these results or to get a 
        copy of the MEME software please access 
        <a href="http://meme.nbcr.net/">http://meme.nbcr.net</a>. 
      </p>
      <p>
        If you use DREME in your research please cite the following paper:<br />
        <span class="citation">
          Timothy L. Bailey, "DREME: Motif discovery in transcription factor ChIP-seq data", <i>Bioinformatics</i>, <b>27</b>(12):1653-1659, 2011.
        </span>
      </p>
    </div>
    <!-- navigation -->
    <div class="pad2">
      <xsl:if test="/dreme/model/description">
      <a class="jump" href="#description">Description</a>
      <xsl:text>&nbsp;&nbsp;|&nbsp;&nbsp;</xsl:text>
      </xsl:if>
      <a class="jump" href="#motifs">Discovered motifs</a>
      <xsl:text>&nbsp;&nbsp;|&nbsp;&nbsp;</xsl:text>
      <a class="jump" href="#program">Program information</a><!--
      <xsl:text>&nbsp;&nbsp;|&nbsp;&nbsp;</xsl:text>
      <a class="jump" href="#doc">Explanation</a>-->
    </div>
  </xsl:template>

  <xsl:template name="description">
    <xsl:if test="/dreme/model/description">
      <xsl:call-template name="util-header">
        <xsl:with-param name="title" select="'Description'"/>
        <xsl:with-param name="self" select="'description'"/>
        <xsl:with-param name="next" select="'motifs'"/>
      </xsl:call-template>
      <div class="box">
        <xsl:call-template name="util-paragraphs">
          <xsl:with-param name="in" select="/dreme/model/description"/>
        </xsl:call-template>
      </div>
    </xsl:if>
  </xsl:template>

  <xsl:template name="motifs">
    <xsl:variable name="desc">
      <xsl:if test="/dreme/model/description">description</xsl:if>
    </xsl:variable>
    <xsl:call-template name="util-header">
      <xsl:with-param name="title" select="'Discovered Motifs'"/>
      <xsl:with-param name="self" select="'motifs'"/>
      <xsl:with-param name="next" select="'program'"/>
      <xsl:with-param name="prev" select="$desc"/>
    </xsl:call-template>
    <div class="box">
      <p> 
        <b>Click on the &more;</b> under the <b>More</b> column to show more 
        information about the motif.<br/>
        <b>Click on the &upload;</b> under the <b>Submit</b> column to send the 
        motif to another MEME suite program. Eg. Tomtom<br/>
        <b>Click on the &download;</b> under the <b>Download</b> column to get 
        the position weight matrix of a motif or to download the logo image with
        your chosen options.
      </p>
      <table id="dreme_motifs" class="dreme_motifs">
        <thead>
        <tr class="motif_head">
          <td>&nbsp;</td>
          <th>Motif <div class="help2" onclick="help(this,'pop_motifs_name')">?</div></th>
          <th>Logo <div class="help2" onclick="help(this,'pop_motifs_logo')">?</div></th>
          <th>RC Logo <div class="help2" onclick="help(this,'pop_motifs_rc_logo')">?</div></th>
          <th>E-value <div class="help2" onclick="help(this,'pop_motifs_evalue')">?</div></th>
          <th>Unerased E-value <div class="help2" onclick="help(this,'pop_motifs_uevalue')">?</div></th>
          <th>More <div class="help2" onclick="help(this,'pop_more')">?</div></th>
          <th>Submit <div class="help2" onclick="help(this,'pop_submit')">?</div></th>
          <th>Download <div class="help2" onclick="help(this,'pop_download')">?</div></th>
        </tr>
        </thead>
        <tbody>
        <xsl:for-each select="/dreme/motifs/motif">
          <xsl:variable name="rcseq">
            <xsl:call-template name="util-rc">
              <xsl:with-param name="input" select="@seq"/>
            </xsl:call-template>
          </xsl:variable>
          <tr class="motif_row" id="motif_row_{position()}">
            <td><xsl:value-of select="position()"/>.</td>
            <td><xsl:value-of select="@seq"/></td>
            <td><img src="{@id}nc_{@seq}.png" id="small_logo_{position()}" 
                width="{@length*15}" height="50"
                onerror="add_draw_task(this, 
                    new FixLogoTask({position()}, false))" />
            </td>
            <td><img src="{@id}rc_{$rcseq}.png" id="small_rc_logo_{position()}" 
                width="{@length*15}" height="50"
                onerror="add_draw_task(this, 
                    new FixLogoTask({position()}, true)); true" />
            </td>
            <td><xsl:value-of select="@evalue"/></td>
            <td><xsl:value-of select="@unerased_evalue"/></td>
            <td class="symaction">
              <a href="javascript:expand({position()})">&more;</a></td>
            <td class="symaction">
              <a href="javascript:send_to_popup({position()})">&upload;</a>
            </td>
            <td class="symaction">
              <a href="javascript:download_popup({position()})">&download;</a>
            </td>
          </tr>
        </xsl:for-each>
        </tbody>
        <tfoot>
          <tr class="rule">
            <td>&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
            <td>&nbsp;</td>
          </tr>
        </tfoot>
      </table>
      <div style="float:left">
        <div onclick="send_to_popup(0);" class="actionbutton"
          title="Submit all motifs to another program.">
          <div style="float:left; margin-right:1em;">Submit All</div>
          <div style="float:right">&upload;</div>
          <div style="clear:both;"></div>
        </div>
      </div>
      <div style="clear:both;"></div>
    </div>
  </xsl:template>

  <xsl:template name="expanded-motif">
    <div id="expanded_motif" style="display:none">
      <div class="box expanded_motif" style="margin-bottom:5px;">
        <div>
          <div style="float:left">
            <h2 class="mainh" style="margin:0; padding:0;">
              <span class="num"></span>.  
              <span class="name"></span>
            </h2>
          </div>
          <div style="float:right; ">
            <div class="close" onclick="contract(this);" title="Show less information.">&less;</div>
          </div>
          <div style="clear:both"></div>
        </div>
        <div style="padding:0 5px;">
          <div style="float:left;">
            <img class="img_nc" onerror="fix_expanded_logo(this, false)"/>
            <img class="img_rc" onerror="fix_expanded_logo(this, true)"/>
          </div>
          <div style="float:right; height:100px;">
            <div onclick="send_to_popup2(this);" class="actionbutton" title="Submit this motif to another MEME Suite program.">
              <div style="float:left; margin-right:1em;">Submit</div>
              <div style="float:right">&upload;</div>
              <div style="clear:both;"></div>
            </div>
            <div onclick="download_popup2(this);" class="actionbutton" title="Download this motif as a position weight matrix or a custom logo.">
              <div style="float:left; margin-right:1em;">Download</div>
              <div style="float:right">&download;</div>
              <div style="clear:both;"></div>
            </div>
          </div>
          <div style="clear:both;"></div>
        </div>
        <h4>Details</h4>
        <table class="details">
          <thead>
            <tr>
              <th>Positives <div class="help2" onclick="help(this,'pop_motif_positives')">?</div></th>
              <th>Negatives <div class="help2" onclick="help(this,'pop_motif_negatives')">?</div></th>
              <th>P-value <div class="help2" onclick="help(this,'pop_motif_pvalue')">?</div></th>
              <th>E-value <div class="help2" onclick="help(this,'pop_motif_evalue')">?</div></th>
              <th>Unerased E-value <div class="help2" onclick="help(this,'pop_motif_uevalue')">?</div></th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td>
                <span class="positives"></span>
                <xsl:text>/</xsl:text>
                <xsl:value-of select="/dreme/model/positives/@count"/>
              </td>
              <td>
                <span class="negatives"></span>
                <xsl:text>/</xsl:text>
                <xsl:value-of select="/dreme/model/negatives/@count"/>
              </td>
              <td class="pvalue"></td>
              <td class="evalue"></td>
              <td class="uevalue"></td>
            </tr>
          </tbody>
        </table>
        <h4>Enriched Matching Words</h4>
        <table>
          <thead>
            <tr>
              <th>
                <a href="javascript:;" 
                  onclick="sort_table(this, compare_strings)">
                  <span class="sort_dir"></span>Word</a>&nbsp;
                <div class="help2" onclick="help(this,'pop_match_word')">?</div>
              </th>
              <th>
                <a href="javascript:;" 
                  onclick="sort_table(this, compare_counts)">
                  <span class="sort_dir"></span>Positives</a>&nbsp;
                <div class="help2" onclick="help(this,'pop_match_pos')">?</div>
              </th>
              <th>
                <a href="javascript:;" 
                  onclick="sort_table(this, compare_counts)">
                  <span class="sort_dir"></span>Negatives</a>&nbsp;
                <div class="help2" onclick="help(this,'pop_match_neg')">?</div>
              </th>
              <th>
                <a href="javascript:;" 
                  onclick="sort_table(this, compare_numbers)">
                  <span class="sort_dir"></span>P-value</a>&nbsp;
                <div class="help2" onclick="help(this,'pop_match_pval')">?</div>
              </th>
              <th>
                <a href="javascript:;" 
                  onclick="sort_table(this, compare_numbers)">
                  <span class="sort_dir">&asc;</span>E-value</a>&nbsp;
                <div class="help2" onclick="help(this,'pop_match_eval')">?</div>
              </th>
            </tr>
          </thead>
          <tbody>
            <tr class="match">
              <td class="dnaseq"></td>
              <td>
                <span class="positives"></span>
                <xsl:text>/</xsl:text>
                <xsl:value-of select="/dreme/model/positives/@count"/>
              </td>
              <td>
                <span class="negatives"></span>
                <xsl:text>/</xsl:text>
                <xsl:value-of select="/dreme/model/negatives/@count"/>
              </td>
              <td class="pvalue"></td>
              <td class="evalue"></td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </xsl:template>

  <xsl:template name="program">
    <a name="program"/>
    <div class="bar">
      <div style="text-align:right;"><a href="#motifs">Previous</a><!--<xsl:text> </xsl:text>-->
        <!--<a href="#doc">Next</a>--><xsl:text> </xsl:text><a href="#top">Top</a></div>
      <div class="subsection">
        <a name="version"/>
        <h5>DREME version</h5>
        <xsl:value-of select="/dreme/@version"/>
        <xsl:text> (Release date: </xsl:text>
        <xsl:value-of select="/dreme/@release"/>)
      </div>
      <div class="subsection">
        <a name="reference"/>
        <h5>Reference</h5>
        <span class="citation">
          Timothy L. Bailey, "DREME: Motif discovery in transcription factor ChIP-seq data", <i>Bioinformatics</i>, <b>27</b>(12):1653-1659, 2011.
        </span>
      </div>
      <div class="subsection">
        <a name="command" />
        <h5>Command line summary</h5>
        <textarea rows="1" style="width:100%;" readonly="readonly">
          <xsl:value-of select="/dreme/model/command_line"/>
        </textarea>
        <br />
        <xsl:text>Result calculation took </xsl:text>
        <xsl:variable name="total_seconds" select="/dreme/run_time/@real"/>
        <xsl:variable name="total_minutes" select="floor($total_seconds div 60)"/>
        <xsl:variable name="hours" select="floor($total_minutes div 60)"/>
        <xsl:variable name="minutes" select="$total_minutes - 60 * $hours"/>
        <!-- don't use mod here as seconds value has fractions of a second too -->
        <xsl:variable name="seconds" select="$total_seconds - 60 * $total_minutes"/>
        <xsl:if test="$hours &gt; 0">
          <xsl:value-of select="$hours"/> hour<xsl:if test="$hours != 1">s</xsl:if>
          <xsl:text> </xsl:text>
        </xsl:if>
        <xsl:if test="$hours &gt; 0 or $minutes &gt; 0">
          <xsl:value-of select="$minutes"/> minute<xsl:if test="$minutes != 1">s</xsl:if>
          <xsl:text> </xsl:text>
        </xsl:if>
        <xsl:value-of select="format-number($seconds, '0.00')"/> second<xsl:if 
          test="$seconds != 1">s</xsl:if>
        <br />
      </div>      
      <a href="javascript:show_hidden('model')" id="model_activator">show model parameters...</a>
      <div class="subsection" id="model_data" style="display:none;">
        <h5>Model parameters</h5>
        <xsl:text>&nl;</xsl:text>
        <textarea style="width:100%;" rows="{count(/dreme/model/*) - 1}" readonly="readonly">
          <xsl:variable name="spaces" select="'                    '"/>
          <xsl:text>&nl;</xsl:text>
          <xsl:for-each select="/dreme/model/*[name(.) != 'command_line' and name(.) != 'description']">
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
            <xsl:text>&nl;</xsl:text>
          </xsl:for-each>
        </textarea>
      </div>
      <a href="javascript:hide_shown('model')" style="display:none;" id="model_deactivator">hide model parameters...</a>
    </div>
  </xsl:template>


  <xsl:template name="dna-pspm">
    <xsl:param name="motif"/>
    <xsl:text>[</xsl:text>
    <xsl:for-each select="$motif/pos">
      <xsl:if test="position() != 1">
        <xsl:text>, </xsl:text>
      </xsl:if>
      <xsl:value-of select="concat('[', @A, ', ', @C, ', ', @G, ', ', @T, ']')"/>
    </xsl:for-each>
    <xsl:text>]</xsl:text>
  </xsl:template>

  <xsl:template name="util-header">
    <xsl:param name="title" />
    <xsl:param name="self" select="$title" />
    <xsl:param name="prev" select="''" />
    <xsl:param name="next" select="''" />

    <a name="{$self}"/>
    <table width="100%" border="0" cellspacing="1" cellpadding="4" bgcolor="#FFFFFF">
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

  <xsl:template name="util-rc">
    <xsl:param name="input"/>
    <xsl:call-template name="util-reverse">
      <xsl:with-param name="input" 
        select="translate($input, 'ACGTURYKMBVDHSWN', 'TGCAAYRMKVBHDSWN')"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="util-reverse">
    <xsl:param name="input"/>
    <xsl:variable name="len" select="string-length($input)"/>
    <xsl:choose>
      <!-- Strings of length less than 2 are trivial to reverse -->
      <xsl:when test="$len &lt; 2">
        <xsl:value-of select="$input"/>
      </xsl:when>
      <!-- Strings of length 2 are also trivial to reverse -->
      <xsl:when test="$len = 2">
        <xsl:value-of select="substring($input,2,1)"/>
        <xsl:value-of select="substring($input,1,1)"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- Swap the recursive application of this template to
            the first half and second half of input -->
        <xsl:variable name="mid" select="floor($len div 2)"/>
        <xsl:call-template name="util-reverse">
          <xsl:with-param name="input"
              select="substring($input,$mid+1,$mid+1)"/>
        </xsl:call-template>
        <xsl:call-template name="util-reverse">
          <xsl:with-param name="input"
              select="substring($input,1,$mid)"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- split into paragraphs separated by one or more empty lines -->
  <xsl:template name="util-paragraphs">
    <xsl:param name="in"/>
    <xsl:choose>
      <xsl:when test="contains($in, '&nl;&nl;')">
        <p>
        <xsl:call-template name="util-lines">
          <xsl:with-param name="in" select="substring-before($in, '&nl;&nl;')"/>
        </xsl:call-template>
        </p>
        <xsl:call-template name="util-paragraphs">
          <xsl:with-param name="in" select="substring-after($in, '&nl;&nl;')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <p>
        <xsl:call-template name="util-lines">
          <xsl:with-param name="in" select="$in"/>
        </xsl:call-template>
        </p>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- split into lines separated by a line break -->
  <xsl:template name="util-lines">
    <xsl:param name="in"/>
    <xsl:choose>
      <xsl:when test="contains($in, '&nl;')">
        <xsl:value-of select="substring-before($in, '&nl;')"/>
        <br />
        <xsl:text>&nl;</xsl:text>
        <xsl:call-template name="util-lines">
          <xsl:with-param name="in" select="substring-after($in, '&nl;')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$in"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
