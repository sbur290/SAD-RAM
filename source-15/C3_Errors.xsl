<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<!-- this file is used by pico_errors.xml to display nicely in a web browser -->

<xsl:template match="/ERROR_MESSAGES">
    <html>
    <head><title>Pico Error Messages</title></head>
    <body>
    <xsl:for-each select="ERROR">
        <h3><xsl:value-of select="@severity"/>&#160;<xsl:value-of select="@number"/> 
                 : <xsl:value-of select="@summary"/></h3>
        <p><xsl:value-of select="."/></p>
        <hr/>
    </xsl:for-each>
    </body>
    </html>
</xsl:template>
</xsl:stylesheet>
