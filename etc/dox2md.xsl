<?xml version="1.0"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  <xsl:template match="/doxygen/compounddef">
    # <xsl:value-of select="compoundname"/>
    <xsl:apply-templates select="innerclass"/>
  </xsl:template>
  <xsl:template match="innerclass">
    <xsl:apply-templates select="sectiondef"/>
  </xsl:template>
  <xsl:template match="sectiondef[@kind='func']">
    <xsl:apply-templates select="memberdef[@kind='function']"/>
  </xsl:template>
  <xsl:template match="sectiondef"/>
  <xsl:template match="memberdef[@kind='function']">
    <xsl:value-of select="definition"/><xsl:value-of select="argsstring"/>
  </xsl:template>
</xsl:stylesheet>
