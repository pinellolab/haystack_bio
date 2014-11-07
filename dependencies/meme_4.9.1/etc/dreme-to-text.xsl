<?xml version="1.0"?>
<!DOCTYPE xsl:stylesheet [
<!ENTITY nl "&#10;">
<!ENTITY tab "&#9;">
]><!-- define nbsp as it is not defined in xml, only lt, gt and amp are defined by default -->
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:cis="http://zlab.bu.edu/schema/cisml"
 xmlns:mem="http://noble.gs.washington.edu/meme"
>
  <xsl:output method="text" />

  <xsl:template match="/dreme">
    <xsl:text># DREME </xsl:text><xsl:value-of select="@version"/><xsl:text>&nl;</xsl:text>
    <xsl:text># command:   </xsl:text><xsl:value-of select="model/command_line"/><xsl:text>&nl;</xsl:text>
    <xsl:text># host:      </xsl:text><xsl:value-of select="model/host"/><xsl:text>&nl;</xsl:text>
    <xsl:text># when:      </xsl:text><xsl:value-of select="model/when"/><xsl:text>&nl;</xsl:text>
    <xsl:text># positives: </xsl:text><xsl:value-of select="model/positives/@count"/><xsl:text>&nl;</xsl:text>
    <xsl:text>#      from: </xsl:text><xsl:value-of select="model/positives/@file"/>
    <xsl:text> (</xsl:text><xsl:value-of select="model/positives/@last_mod_date"/><xsl:text>)&nl;</xsl:text>
    <xsl:text># negatives: </xsl:text><xsl:value-of select="model/negatives/@count"/><xsl:text>&nl;</xsl:text>
    <xsl:text>#      from: </xsl:text>
    <xsl:choose>
      <xsl:when test="model/negatives/@from = 'shuffled'">
        <xsl:text>shuffled positives&nl;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="model/negatives/@file"/>
        <xsl:text> (</xsl:text><xsl:value-of select="model/negatives/@last_mod_date"/><xsl:text>)&nl;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="model/description">
      <xsl:text>#&nl;</xsl:text> 
      <xsl:call-template name="description"/>
    </xsl:if>
    <xsl:text>&nl;&nl;</xsl:text>
    <xsl:text>MEME version </xsl:text><xsl:value-of select="@version"/>
    <xsl:text>&nl;&nl;</xsl:text>
    <xsl:text>ALPHABET= </xsl:text>
    <xsl:choose>
      <xsl:when test="model/background/@type = 'dna'">
        <xsl:text>ACGT</xsl:text>
      </xsl:when>
      <xsl:when test="model/background/@type = 'rna'">
        <xsl:text>ACGU</xsl:text>
      </xsl:when>
      <xsl:when test="model/background/@type = 'aa'">
        <xsl:text>ACDEFGHIKLMNPQRSTVWY</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">Unknown alphabet type!</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&nl;&nl;</xsl:text>
    <xsl:if test="model/background/@type = 'dna'">
      <xsl:text>strands: + -&nl;&nl;</xsl:text>
    </xsl:if>
    <xsl:text>Background letter frequencies (from </xsl:text>
    <xsl:choose>
      <xsl:when test="model/background/@from = 'dataset'">
        <xsl:text>dataset):</xsl:text>
      </xsl:when>
      <xsl:when test="model/background/@from = 'file'">
        <xsl:value-of select="model/background/@file"/><xsl:text>):</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">Unknown background source!</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&nl;</xsl:text>
    <xsl:choose>
      <xsl:when test="model/background/@type = 'dna'">
        <xsl:text>A </xsl:text>
        <xsl:value-of select="model/background/@A"/>
        <xsl:text> C </xsl:text>
        <xsl:value-of select="model/background/@C"/>
        <xsl:text> G </xsl:text>
        <xsl:value-of select="model/background/@G"/>
        <xsl:text> T </xsl:text>
        <xsl:value-of select="model/background/@T"/>
      </xsl:when>
      <xsl:when test="model/background/@type = 'rna'">
        <xsl:text>A </xsl:text>
        <xsl:value-of select="model/background/@A"/>
        <xsl:text> C </xsl:text>
        <xsl:value-of select="model/background/@C"/>
        <xsl:text> G </xsl:text>
        <xsl:value-of select="model/background/@G"/>
        <xsl:text> U </xsl:text>
        <xsl:value-of select="model/background/@U"/>
      </xsl:when>
      <xsl:when test="model/background/@type = 'aa'">
        <xsl:text>A </xsl:text>
        <xsl:value-of select="model/background/@A"/>
        <xsl:text> C </xsl:text>
        <xsl:value-of select="model/background/@C"/>
        <xsl:text> D </xsl:text>
        <xsl:value-of select="model/background/@D"/>
        <xsl:text> E </xsl:text>
        <xsl:value-of select="model/background/@E"/>
        <xsl:text> F </xsl:text>
        <xsl:value-of select="model/background/@F"/>
        <xsl:text> G </xsl:text>
        <xsl:value-of select="model/background/@G"/>
        <xsl:text> H </xsl:text>
        <xsl:value-of select="model/background/@H"/>
        <xsl:text> I </xsl:text>
        <xsl:value-of select="model/background/@I"/>
        <xsl:text> K </xsl:text>
        <xsl:value-of select="model/background/@K"/>
        <xsl:text> L </xsl:text>
        <xsl:value-of select="model/background/@L"/>
        <xsl:text> M </xsl:text>
        <xsl:value-of select="model/background/@M"/>
        <xsl:text> N </xsl:text>
        <xsl:value-of select="model/background/@N"/>
        <xsl:text> P </xsl:text>
        <xsl:value-of select="model/background/@P"/>
        <xsl:text> Q </xsl:text>
        <xsl:value-of select="model/background/@Q"/>
        <xsl:text> R </xsl:text>
        <xsl:value-of select="model/background/@R"/>
        <xsl:text> S </xsl:text>
        <xsl:value-of select="model/background/@S"/>
        <xsl:text> T </xsl:text>
        <xsl:value-of select="model/background/@T"/>
        <xsl:text> V </xsl:text>
        <xsl:value-of select="model/background/@V"/>
        <xsl:text> W </xsl:text>
        <xsl:value-of select="model/background/@W"/>
        <xsl:text> Y </xsl:text>
        <xsl:value-of select="model/background/@Y"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">Unknown alphabet type!</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&nl;&nl;</xsl:text>

    <xsl:for-each select="motifs/motif">
      <xsl:call-template name="motif"/>
    </xsl:for-each>

    <xsl:text>&nl;Time </xsl:text>
    <xsl:value-of select="/dreme/run_time/@real"/>
    <xsl:text> secs.&nl;</xsl:text>

  </xsl:template>

  <xsl:template name="description">
    <xsl:param name="in" select="/dreme/model/description"/>
    <xsl:choose>
      <xsl:when test="contains($in, '&nl;')">
        <xsl:text># </xsl:text>
        <xsl:value-of select="substring-before($in, '&nl;')"/>
        <xsl:text>&nl;</xsl:text>
        <xsl:call-template name="description">
          <xsl:with-param name="in" select="substring-after($in, '&nl;')"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text># </xsl:text>
        <xsl:value-of select="$in"/>
        <xsl:text>&nl;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="motif">
    <xsl:text>MOTIF </xsl:text>
    <xsl:choose>
      <xsl:when test="@name">
        <xsl:value-of select="@name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="@seq"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="@alt">
      <xsl:text> </xsl:text>
      <xsl:value-of select="@alt"/>
    </xsl:if>
    <xsl:text>&nl;&nl;</xsl:text>
    <xsl:variable name="spaces" select="'          '"/>
    <xsl:text>#             Word    RC Word        Pos        Neg    P-value    E-value&nl;</xsl:text>
    <xsl:text># BEST </xsl:text>
    <xsl:value-of select="substring($spaces,string-length(@seq))"/>
    <xsl:value-of select="@seq"/>
    <xsl:value-of select="substring($spaces,string-length(@seq))"/>
    <xsl:call-template name="util-rc"><xsl:with-param name="input" select="@seq"/></xsl:call-template>
    <xsl:value-of select="substring($spaces,string-length(@p))"/>
    <xsl:value-of select="@p"/>
    <xsl:value-of select="substring($spaces,string-length(@n))"/>
    <xsl:value-of select="@n"/>
    <xsl:value-of select="substring($spaces,string-length(@pvalue))"/>
    <xsl:value-of select="@pvalue"/>
    <xsl:value-of select="substring($spaces,string-length(@evalue))"/>
    <xsl:value-of select="@evalue"/>
    <xsl:text>&nl;</xsl:text>
    <xsl:for-each select="match">
      <xsl:text>#      </xsl:text>
      <xsl:value-of select="substring($spaces,string-length(@seq))"/>
      <xsl:value-of select="@seq"/>
      <xsl:value-of select="substring($spaces,string-length(@seq))"/>
      <xsl:call-template name="util-rc"><xsl:with-param name="input" select="@seq"/></xsl:call-template>
      <xsl:value-of select="substring($spaces,string-length(@p))"/>
      <xsl:value-of select="@p"/>
      <xsl:value-of select="substring($spaces,string-length(@n))"/>
      <xsl:value-of select="@n"/>
      <xsl:value-of select="substring($spaces,string-length(@pvalue))"/>
      <xsl:value-of select="@pvalue"/>
      <xsl:value-of select="substring($spaces,string-length(@evalue))"/>
      <xsl:value-of select="@evalue"/>
      <xsl:text>&nl;</xsl:text>
    </xsl:for-each>
    <xsl:text>&nl;</xsl:text>
    <xsl:text>letter-probability matrix: alength= </xsl:text>
    <xsl:choose>
      <xsl:when test="/dreme/model/background/@type = 'dna'">
        <xsl:text>4</xsl:text>
      </xsl:when>
      <xsl:when test="/dreme/model/background/@type = 'rna'">
        <xsl:text>4</xsl:text>
      </xsl:when>
      <xsl:when test="/dreme/model/background/@type = 'aa'">
        <xsl:text>20</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">Unknown alphabet type!</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text> w= </xsl:text>
    <xsl:value-of select="@length"/>
    <xsl:text> nsites= </xsl:text>
    <xsl:value-of select="@nsites"/>
    <xsl:text> E= </xsl:text>
    <xsl:value-of select="@evalue"/>
    <xsl:text>&nl;</xsl:text>
    <xsl:choose>
      <xsl:when test="/dreme/model/background/@type = 'dna'">
        <xsl:for-each select="pos">
          <xsl:sort select="@i" data-type="number" order="ascending"/>
          <xsl:value-of select="@A"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@C"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@G"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@T"/>
          <xsl:text>&nl;</xsl:text>
        </xsl:for-each>
      </xsl:when>
      <xsl:when test="/dreme/model/background/@type = 'rna'">
        <xsl:for-each select="pos">
          <xsl:sort select="@i" data-type="number" order="ascending"/>
          <xsl:value-of select="@A"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@C"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@G"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@U"/>
          <xsl:text>&nl;</xsl:text>
        </xsl:for-each>
      </xsl:when>
      <xsl:when test="/dreme/model/background/@type = 'aa'">
        <xsl:for-each select="pos">
          <xsl:sort select="@i" data-type="number" order="ascending"/>
          <xsl:value-of select="@A"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@C"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@D"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@E"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@F"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@G"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@H"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@I"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@K"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@L"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@M"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@N"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@P"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@Q"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@R"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@S"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@T"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@V"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@W"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@Y"/>
          <xsl:text>&nl;</xsl:text>
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">Unknown alphabet type!</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&nl;&nl;</xsl:text>
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

</xsl:stylesheet>
