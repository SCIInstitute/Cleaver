<h2>{{ page.project }}</h3>

<p>{{ page.project }} (<a href={{ page.releaseurl }}> {{ page.releaseurl }}</a>) is a free multimaterial tetrahedral meshing tool developed by the NIH Center for Integrative Biomedical Computing at the University of Utah Scientific Computing and Imaging (SCI) Institute.</p>

<h3>Installation</h3>

<p>Check the <a href={{ site.github.url }}/specs.html>Platform Specifications</a> for system requirements.</p>

<p><a href={{ page.releaseurl }}>Installers</a> are provided for Windows and Mac OS X. Linux users need to build {{ page.project }} from <a href={{ site.github.url }}/build.html>source</a>.</p>

<h3>Documentation</h3>
<ul>
  {% for page in site.pages %}
  {% if page.category == "info" %}
  <h3> <a href={{ site.github.url }}{{ page.url }}>{{ page.title }}</a></h3>
  {% endif %}
  {% endfor %}
  <h3><a href={{ site.github.url }}/doxygen/index.html>Doxygen Code Reference</a></h3>
</ul>
