<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:cis="http://zlab.bu.edu/schema/cisml"
 xmlns:mem="http://noble.gs.washington.edu/meme"
>
  <xsl:output method="text" />

  <!-- A stylesheet to create a BED file from MCAST CisML.-->

  <xsl:template match="/*">
    <xsl:apply-templates select="cis:multi-pattern-scan/mem:match">
      <xsl:sort order="descending" data-type="text" select="@seq-name"/>
      <xsl:sort order="ascending" data-type="number" select="@start"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="mem:match">
   <!-- sequence name -->
   <xsl:value-of select="@seq-name"/>
   <xsl:text>&#9;</xsl:text>
   <!-- start -->
   <xsl:value-of select="@start"/>
   <xsl:text>&#9;</xsl:text>
   <!-- end -->
   <xsl:value-of select="@stop"/>
   <xsl:text>&#9;</xsl:text>
   <!-- name -->
   <xsl:value-of select="@cluster-id"/>
   <xsl:text>&#9;</xsl:text>
   <!-- 1000 * q-value -->
   <xsl:value-of select="1000 * (1.0 - number(@qvalue))"/>
   <xsl:text>&#10;</xsl:text>
  </xsl:template>

</xsl:stylesheet>
