<?xml version="1.0"?>
<!-- A stylesheet to create a plain text tab delimited file from FIMO CisML.-->
<xsl:stylesheet
 version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:cis="http://zlab.bu.edu/schema/cisml"
 xmlns:mem="http://noble.gs.washington.edu/meme"
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
    <xsl:text>#pattern name&#9;sequence name&#9;start&#9;stop&#9;strand&#9;score&#9;p-value&#9;q-value&#9;matched sequence&#10;</xsl:text>
    <xsl:apply-templates select="cis:pattern/cis:scanned-sequence/cis:matched-element">
      <xsl:sort order="ascending" data-type="number" select="@pvalue"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="cis:matched-element">
   <!-- pattern name -->
   <xsl:value-of select="../../@name"/><xsl:text>&#9;</xsl:text>
   <!-- sequence name -->
   <xsl:value-of select="../@name"/>
   <xsl:text>&#9;</xsl:text>
   <!-- start  and stop positions -->
   <!-- CISML uses start > stop to indicate - strand -->
   <!-- Text always wants start < stop, indicate strand in strand column -->
   <xsl:choose>
     <xsl:when test="@start &lt; @stop">
       <!-- start < stop so + strand -->
       <xsl:value-of select="@start"/>
       <xsl:text>&#9;</xsl:text>
       <!-- end -->
       <xsl:value-of select="@stop"/>
       <xsl:text>&#9;</xsl:text>
       <!-- strand -->
       <xsl:choose>
         <xsl:when test="$alphabet='nucleotide'">
            <xsl:text>+&#9;</xsl:text>
         </xsl:when>
         <xsl:otherwise>
            <xsl:text>.&#9;</xsl:text>
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
       <!-- strand -->
       <xsl:choose>
         <xsl:when test="$alphabet='nucleotide'">
            <xsl:text>-&#9;</xsl:text>
         </xsl:when>
         <xsl:otherwise>
            <xsl:text>.&#9;</xsl:text>
         </xsl:otherwise>
       </xsl:choose>
     </xsl:otherwise>
   </xsl:choose>
   <!-- score -->
   <xsl:value-of select="@score"/>
   <xsl:text>&#9;</xsl:text>
   <!-- p-value -->
   <xsl:value-of select="@pvalue"/>
   <xsl:text>&#9;</xsl:text>
   <!-- q-value -->
   <xsl:value-of select="./mem:qvalue"/>
   <xsl:text>&#9;</xsl:text>
   <!-- Matched sequence -->
   <xsl:value-of select="./cis:sequence"/>
   <xsl:text>&#10;</xsl:text>
  </xsl:template>

</xsl:stylesheet>
