<?xml version="1.0"?>
<!-- A stylesheet to create a GFF3 formated file from FIMO CisML.-->
<xsl:stylesheet
 version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:cis="http://zlab.bu.edu/schema/cisml"
 xmlns:exsl="http://exslt.org/common"
 xmlns:func="http://exslt.org/functions"
 xmlns:mem="http://noble.gs.washington.edu/meme"
 xmlns:math="http://exslt.org/math"
 extension-element-prefixes="func"
>
  <xsl:output method="text"/>

  <xsl:template match="/fimo">
    <xsl:apply-templates select="cisml-file"/>
  </xsl:template>

  <xsl:param name="alphabet" select="/fimo/alphabet"/>

  <xsl:template match="cisml-file">
    <xsl:apply-templates select="document(text())"/>
  </xsl:template>

  <xsl:template match="cis:cis-element-search">
    <xsl:text>##gff-version 3&#10;</xsl:text>
    <xsl:apply-templates select="cis:pattern/cis:scanned-sequence/cis:matched-element">
      <xsl:sort order="ascending" data-type="text" select="../@name"/>
      <xsl:sort order="ascending" data-type="text" select="../../@name"/>
      <xsl:sort order="ascending" data-type="number" select="mem:min(@start, @stop)"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="cis:matched-element">
   <!-- sequence name -->
   <xsl:value-of select="../@name"/>
   <xsl:text>&#9;</xsl:text>
   <!-- source -->
   <xsl:value-of select="/cis:cis-element-search/cis:program-name" />
   <xsl:text>&#9;</xsl:text>
   <!-- feature-->
   <xsl:choose>
     <xsl:when test="$alphabet='nucleotide'">
        <xsl:text>nucleotide_motif</xsl:text>
     </xsl:when>
     <xsl:otherwise>
        <xsl:text>polypeptide_motif</xsl:text>
     </xsl:otherwise>
   </xsl:choose>
   <xsl:text>&#9;</xsl:text>
   <!-- start  and stop positions -->
   <!-- CISML uses start > stop to indicate - strand -->
   <!-- GFF always wants start < stop, indicate strand in strand column -->
   <xsl:choose>
     <xsl:when test="@start &lt; @stop">
       <!-- start < stop so + strand -->
       <xsl:value-of select="@start"/>
       <xsl:text>&#9;</xsl:text>
       <!-- end -->
       <xsl:value-of select="@stop"/>
       <xsl:text>&#9;</xsl:text>
       <!-- score -->
       <!-- math:log() is the natual log, so we write (-10 * log10(x)) as (-4.342 * ln(x)) -->
       <xsl:value-of select="format-number(mem:min(1000, (-4.342 * math:log(@pvalue))), '####.0')"/>
       <!-- strand -->
       <xsl:choose>
         <xsl:when test="$alphabet='nucleotide'">
            <xsl:text>&#9;+</xsl:text>
         </xsl:when>
         <xsl:otherwise>
            <xsl:text>&#9;.</xsl:text>
         </xsl:otherwise>
       </xsl:choose>
     </xsl:when>
     <xsl:otherwise>
       <!-- start > stop so - strand -->
       <!-- end -->
       <xsl:value-of select="@stop"/>
       <xsl:text>&#9;</xsl:text>
       <!-- start -->
       <xsl:value-of select="@start"/>
       <xsl:text>&#9;</xsl:text>
       <!-- score -->
       <!-- math:log() is the natual log, so we write (-10 * log10(x)) as (-4.342 * ln(x)) -->
       <xsl:value-of select="format-number(mem:min(1000, (-4.342 * math:log(@pvalue))), '####.0')"/>
       <!-- strand -->
       <xsl:choose>
         <xsl:when test="$alphabet='nucleotide'">
            <xsl:text>&#9;-</xsl:text>
         </xsl:when>
         <xsl:otherwise>
            <xsl:text>&#9;.</xsl:text>
         </xsl:otherwise>
       </xsl:choose>
     </xsl:otherwise>
   </xsl:choose>
   <!-- frame -->
   <xsl:text>&#9;.</xsl:text>
   <!-- attributes -->
   <xsl:text>&#9;</xsl:text>
   <!-- attribute motif_name -->
   <xsl:text>Name=</xsl:text>
   <xsl:value-of select="../../@name"/>
   <xsl:text>;</xsl:text>
   <xsl:text>ID=</xsl:text>
   <xsl:value-of select="../../@name"/>
   <xsl:text>-</xsl:text>
   <xsl:number count="cis:matched-element"/>
   <xsl:text>;</xsl:text>
   <!-- attribute pvalue -->
   <xsl:text>pvalue=</xsl:text>
   <xsl:value-of select="@pvalue"/>
   <xsl:text>;</xsl:text>
   <!-- attribute qvalue -->
   <xsl:if test="./mem:qvalue/text() &gt; 0">
     <xsl:text>qvalue=</xsl:text>
     <xsl:value-of select="./mem:qvalue/text()"/>
     <xsl:text>;</xsl:text>
   </xsl:if>
   <!-- attribute sequence -->
   <xsl:text>sequence=</xsl:text>
   <xsl:value-of select="./cis:sequence"/>
   <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <func:function name="mem:min">
    <xsl:param name="e1"/>
    <xsl:param name="e2"/>

    <xsl:variable name="x">
      <xsl:choose>
        <xsl:when test="number($e1) &gt; number($e2)">
          <xsl:value-of select="$e2"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$e1"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <func:result select="$x" />

  </func:function>

  <func:function name="mem:max">
    <xsl:param name="e1"/>
    <xsl:param name="e2"/>

    <xsl:variable name="x">
      <xsl:choose>
        <xsl:when test="number($e1) &gt; number($e2)">
          <xsl:value-of select="$e1"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$e2"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <func:result select="$x" />

  </func:function>

</xsl:stylesheet>
