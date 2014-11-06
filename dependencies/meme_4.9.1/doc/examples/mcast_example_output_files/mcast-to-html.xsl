<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE xsl:stylesheet [
<!ENTITY nbsp "&#160;">
<!ENTITY space " ">
<!ENTITY newline "&#10;">
<!ENTITY tab "&#9;">
<!ENTITY more "&#8615;">
]><!-- define nbsp as it is not defined in xml, only lt, gt and amp are defined by default -->
<xsl:stylesheet version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:cis="http://zlab.bu.edu/schema/cisml"
  xmlns:mem="http://noble.gs.washington.edu/meme"
>

  <xsl:output method="html" indent="yes" 
    doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
    doctype-system="http://www.w3.org/TR/html4/loose.dtd"
    />
  <xsl:include href="meme.css.xsl"/>
  <xsl:include href="block-diagram.xsl"/>

  <!-- useful variables -->

  <xsl:variable name="max_seq_len">
    <xsl:for-each select="document('cisml.xml')/cis:cis-element-search/cis:multi-pattern-scan/mem:match" >
      <xsl:sort select="@stop - @start" data-type="number" order="descending"/>
      <xsl:if test="position() = 1">
        <xsl:value-of select="@stop - @start + 1" />
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="max_log_pvalue">
    <xsl:call-template name="ln-approx">
      <xsl:with-param name="base" select="0.0000000001"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="longest_seq_name">
    <xsl:for-each select="document('cisml.xml')/cis:cis-element-search/cis:multi-pattern-scan/mem:match" >
      <xsl:sort select="string-length(@seq-name)" data-type="number" order="descending"/>
      <xsl:if test="position() = 1">
        <xsl:choose>
          <xsl:when test="string-length(@seq-name) &gt; string-length('Sequence')">
            <xsl:value-of select="string-length(@seq-name)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="string-length('Sequence')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="seq_is_dna">
    <xsl:choose>
      <xsl:when test="/mcast/alphabet/text() = 'nucleotide'">
        <xsl:text>true</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>false</xsl:text>
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
      <meta name="description" content="Motif Cluster Alignment and Search Tool (MCAST) output."/>
      <title>MCAST</title>
      <script type="text/javascript">
        var motifs = new Array();<xsl:text>&newline;</xsl:text>
        <xsl:for-each select="/mcast/motif">
          <xsl:text>        motifs[&quot;</xsl:text><xsl:value-of select='@name'/><xsl:text>&quot;] = new Motif(&quot;</xsl:text>
          <xsl:value-of select='@name'/><xsl:text>&quot;, &quot;</xsl:text>
          <xsl:value-of select='/mcast/alphabet/text()'/><xsl:text>&quot;, &quot;</xsl:text>
          <xsl:value-of select='@best-possible-match'/><xsl:text>&quot;, &quot;</xsl:text>
          <xsl:value-of select='@best-possible-rc-match'/><xsl:text>&quot;);&newline;</xsl:text>
        </xsl:for-each>

        var wrap = undefined;//size to display on one line
        var wrap_timer;

        var seqmax = <xsl:value-of select="$max_seq_len"/>;

        var loadedSequences = new Array();
        //draging details
        var moving_seq;
        var moving_annobox;
        var moving_left;
        var moving_width;
        var moving_both;

        //drag needles
        var dnl = null;
        var dnr = null;
        var drag_is_rc = undefined;
        //container
        var cont = null;
        
        function mouseCoords(ev) {
          ev = ev || window.event;
          if(ev.pageX || ev.pageY){ 
	          return {x:ev.pageX, y:ev.pageY}; 
	        } 
	        return { 
	          x:ev.clientX + document.body.scrollLeft - document.body.clientLeft, 
	          y:ev.clientY + document.body.scrollTop  - document.body.clientTop 
	        };
        }

        function setup() {
          rewrap();
          window.onresize = delayed_rewrap;
        }

        function calculate_wrap() {
          var est_wrap = 0;
          var page_width;
          if (window.innerWidth) {
            page_width = window.innerWidth;
          } else if (document.body) {
            page_width = document.body.clientWidth;
          } else {
            page_width = null;
          }
          if (page_width) {
            var ruler_width = document.getElementById("ruler").offsetWidth;
            est_wrap = Math.floor(page_width / ruler_width) * 5;
          }
          if (est_wrap > 20) {
            wrap = est_wrap - 20; 
          } else {
            wrap = 100;
          }
        }

        function rewrap() {
          var previous_wrap = wrap;
          calculate_wrap();
          if (previous_wrap != wrap) {
            for (var seqid in loadedSequences) {
              //exclude inherited properties and undefined properties
              if (!loadedSequences.hasOwnProperty(seqid) || loadedSequences[seqid] === undefined) continue;
            
              var sequence = loadedSequences[seqid];
              var annobox = document.getElementById(seqid + "_annotation");
              var leftPos = parseInt(document.getElementById(seqid + "_dnl").firstChild.firstChild.nodeValue);
              var rightPos = parseInt(document.getElementById(seqid + "_dnr").firstChild.firstChild.nodeValue);
              var is_rc = undefined;&newline;
              set_data(leftPos, rightPos - leftPos + 1, is_rc, sequence, annobox);
            }
          }
          if (wrap_timer) {
            clearTimeout(wrap_timer);
          }
          wrap_timer = setTimeout("rewrap()", 5000);
        }

        function delayed_rewrap() {
          if (wrap_timer) {
            clearTimeout(wrap_timer);
          }
          wrap_timer = setTimeout("rewrap()", 1000);
        }

        function showHidden(prefix) {
          document.getElementById(prefix + '_activator').style.display = 'none';
          document.getElementById(prefix + '_deactivator').style.display = 'block';
          document.getElementById(prefix + '_data').style.display = 'block';
        }
        function hideShown(prefix) {
          document.getElementById(prefix + '_activator').style.display = 'block';
          document.getElementById(prefix + '_deactivator').style.display = 'none';
          document.getElementById(prefix + '_data').style.display = 'none';
        }


        function show_more(seqid) {
          if (wrap == undefined) rewrap();
          
          var seq_row = document.getElementById(seqid + "_blocks");
          var info_row = document.getElementById(seqid + "_info");
          var tbl = document.getElementById("tbl_sequences");
          if (info_row) {
            tbl.deleteRow(info_row.rowIndex);
            destroy_seq_handle(seqid, true);
            destroy_seq_handle(seqid, false);
            delete loadedSequences[seqid];
          } else {
            var sequence = new Sequence(seqid);
            var offset = sequence.segs[0].start;
            loadedSequences[seqid] = sequence;
            info_row = tbl.insertRow(seq_row.rowIndex + 1);
            info_row.id = seqid + "_info";
            var cell = info_row.insertCell(0);
            cell.colSpan = 8;
            cell.style.verticalAlign = "top";
            var width = Math.min(sequence.length, wrap); 
            var start = 1;
            var is_neg_strand = false;
            //center on the first hit
            if (sequence.hits.length &gt; 0) {
              var hit = sequence.hits[0];
              is_neg_strand = hit.is_rc;
              start = hit.pos - offset;
            }
            if (start + width &gt;= sequence.length) start = sequence.length - width;
            create_info_section(cell, sequence, seqid, start+offset, width, is_neg_strand);
            var blockCont = document.getElementById(seqid + "_block_container");
            //create sequence view handles
            var dnl = create_seq_handle(blockCont, seqid, true);
            var dnr = create_seq_handle(blockCont, seqid, false);
            set_needles(offset, seqmax, dnl, start, dnr, start + width - 1);
          }
        }

        function create_info_section(cell, sequence, seqid, start, width, is_neg_strand) {
          var box = document.createElement('div');
          box.className = "box infobox";
          //generic info
          box.appendChild(document.createTextNode("Change the portion of annotated sequence by "));
          var bold_box = document.createElement('b');
          bold_box.appendChild(document.createTextNode("dragging the buttons"));
          box.appendChild(bold_box);
          box.appendChild(document.createTextNode("; hold shift to drag them individually."));
          box.appendChild(document.createElement('br'));
          box.appendChild(document.createElement('br'));
          //sequence display
          var seqDispTitle = document.createElement('h5');
          seqDispTitle.appendChild(document.createTextNode("Annotated Sequence"));
          box.appendChild(seqDispTitle);
          var show_rc_only = undefined;
          var annobox = document.createElement('div');
          annobox.id = seqid + "_annotation";
          set_data(start, width, show_rc_only, sequence, annobox);
          box.appendChild(annobox);
          cell.appendChild(box);
        }

        function annobox_labels() {
          var seqDispLabel = document.createElement('div');
          seqDispLabel.className = "sequence sequence_labels";
          seqDispLabel.style.height = "2.5em";
          return seqDispLabel;
        }

        function annobox_hits() {
          var seqDispHit = document.createElement('div');
          seqDispHit.className = "sequence";
          seqDispHit.style.height = "1.5em";
          return seqDispHit;
        }

        function annobox_matches() {
          var seqDispMatch = document.createElement('div');
          seqDispMatch.className = "sequence";
          seqDispMatch.style.height = "1.5em";
          return seqDispMatch;
        }

        function annobox_translations() {
          var seqDispXlate = document.createElement('div');
          seqDispXlate.className = "sequence";
          seqDispXlate.style.height = "1.5em";
          return seqDispXlate;
        }

        function annobox_sequence() {
          var seqDispSeq = document.createElement('div');
          seqDispSeq.className = "sequence";
          seqDispSeq.style.height = "1.5em";
          return seqDispSeq; 
        }
        function annobox_boundary() {
          var seqDispSeq = document.createElement('div');
          seqDispSeq.className = "sequence";
          seqDispSeq.style.height = "1.5em";
          return seqDispSeq; 
        }

        function create_seq_handle(container, seqid, isleft) {
          var vbar = document.createElement('div');
          vbar.id = seqid + "_dn" + (isleft ? "l" : "r");
          vbar.className = "block_needle" + (isleft ? "" : " right");
          var label = document.createElement('div');
          label.className = "block_handle" + (isleft ? "" : " right");
          label.appendChild(document.createTextNode("1"));
          label.style.cursor = 'pointer';
          label.title = "Drag to move the displayed range. Hold shift and drag to change " + (isleft ? "lower" : "upper") + " bound of the range.";
          vbar.appendChild(label);
          container.appendChild(vbar);
          label.onmousedown = function(evt) {
            evt = evt || window.event;
            start_drag(seqid, isleft, !(evt.shiftKey));
          };
          return vbar;
        }

        function destroy_seq_handle(seqid, isleft) {
          var vbar = document.getElementById(seqid + "_dn" + (isleft ? "l" : "r"));
          var label = vbar.firstChild;
          label.onmousedown = null;
          vbar.parentNode.removeChild(vbar);
        }

        function start_drag(seqid, mouse_on_left, move_both) {
          //first record what we are moving
          moving_left = mouse_on_left;
          moving_both = move_both;
          moving_seq = loadedSequences[seqid];
          if (!moving_seq) {
            alert("Sequence not loaded?!");
          }
          //get the container for movements to be judged against
          cont = document.getElementById(seqid + "_block_container");
          //get the div's that will be moved
          dnl = document.getElementById(seqid + "_dnl");
          dnr = document.getElementById(seqid + "_dnr");
          //get the div which has all the text containers that will be updated
          moving_annobox = document.getElementById(seqid + "_annotation");
          //calculate the space between handles
          moving_width = dnr.firstChild.firstChild.nodeValue - 
              dnl.firstChild.firstChild.nodeValue;
          drag_is_rc = undefined;
          //setup the events for draging
          document.onmousemove = handle_drag;
          document.onmouseup = finish_drag;
        }


        function calculate_width(obj) {
          return obj.clientWidth - (obj.style.paddingLeft ? obj.style.paddingLeft : 0) 
              - (obj.style.paddingRight ? obj.style.paddingRight : 0);
        }

        function calculate_drag_pos(ev) {
          var mouse = mouseCoords(ev);
          var dragable_length = calculate_width(cont);
          //I believe that the offset parent is the body
          //otherwise I would need to make this recursive
          //maybe clientLeft would work, but the explanation of
          //it is hard to understand and it apparently doesn't work
          //in firefox 2.
          var diff = mouse.x - cont.offsetLeft;
          if (diff &lt; 0) diff = 0;
          if (diff &gt; dragable_length) diff = dragable_length;
          var pos = Math.round(diff / dragable_length * (seqmax));
          return pos;
        }

        function handle_drag(ev) {
          var pos = calculate_drag_pos(ev);
          update_needles(pos, moving_seq.length, false);
        }

        function finish_drag(ev) {
          document.onmousemove = null;
          document.onmouseup = null;
          var pos = calculate_drag_pos(ev);
          update_needles(pos, moving_seq.length, true);
        }

        function update_needles(pos, seqlen, updateTxt) {
          var offset = moving_seq.segs[0].start;
          var leftPos = parseInt(dnl.firstChild.firstChild.nodeValue) - offset;
          var rightPos = parseInt(dnr.firstChild.firstChild.nodeValue) - offset;
          if (moving_both) {
            if (moving_left) {
              if ((pos + moving_width) &gt;= seqlen) {
                pos = seqlen - moving_width - 1;
              }
              leftPos = pos;
              rightPos = pos + moving_width;
            } else {
              if (pos &gt;= seqlen) {
                pos = seqlen - 1;
              } else if ((pos - moving_width) &lt; 0) {
                pos = moving_width;
              }
              leftPos = pos - moving_width;
              rightPos = pos;
            }
          } else {
            if (moving_left) {
              if (pos &gt; rightPos) {
                pos = rightPos;
              }
              leftPos = pos;
            } else {
              if (pos &lt; leftPos) {
                pos = leftPos;
              } else if (pos &gt;= seqlen) {
                pos = seqlen - 1;
              }
              rightPos = pos;
            }
          }
          set_needles(offset, seqmax, dnl, leftPos, dnr, rightPos);
          if (updateTxt) set_data(leftPos + offset, rightPos - leftPos + 1, drag_is_rc, moving_seq, moving_annobox);
        }

        function set_needles(offset, max, left, leftPos, right, rightPos) {
          //the needles are between the sequence positions
          //the left needle is before and the right needle after
          //think of it as an inclusive range...
          left.style.left = "" + (leftPos / max * 100)+ "%";
          left.firstChild.firstChild.nodeValue="" + (leftPos + offset);
          right.style.left = "" + ((rightPos+1) / max * 100)+ "%";
          right.firstChild.firstChild.nodeValue="" + (rightPos + offset);
        }

        function set_data(start, width, show_rc_only, data, annobox) {
          var child = annobox.firstChild;
          var line_width = Math.min(wrap, width);
          var num_per_wrap = 5
          var end = start + width;
          for (var i = start; i &lt; end; i += line_width, line_width = Math.min(wrap, end - i)) {
            for (var j = 0; j &lt; num_per_wrap; ++j) {
              if (child) {
                while (child.firstChild) child.removeChild(child.firstChild);
              } else {
                switch (j) {
                case 0:
                  child = annobox_labels();
                  break;
                case 1:
                  child = annobox_hits();
                  break;
                case 2:
                  child = annobox_matches();
                  break;
                case 3:
                  child = annobox_sequence();
                  break;
                case 4:
                  child = annobox_boundary();
                  break;
                }
                annobox.appendChild(child);
              }
              switch (j) {
              case 0:
                data.append_labels(child, i, line_width, show_rc_only);
                break;
              case 1:
                data.append_hits(child, i, line_width, show_rc_only);
                break;
              case 2:
                data.append_matches(child, i, line_width, show_rc_only);
                break;
              case 3:
                data.append_seq(child, i, line_width, show_rc_only);
                break;
              case 4:
                data.append_boundary(child, i, line_width, show_rc_only);
                break;
              }
              child = child.nextSibling;
            }
          }
          //clean up excess
          
          while (child) {
            var next = child.nextSibling;
            annobox.removeChild(child);
            child = next;
          }
        }

        function append_coloured_nucleotide_sequence(container, sequence) {
          var len = sequence.length;
          for (var i = 0; i &lt; len; i++) {
            var colour = "black";
            switch (sequence.charAt(i)) {
            case 'A':
              colour = "red";
              break;
            case 'C':
              colour = "blue";
              break;
            case 'G':
              colour = "orange";
              break;
            case 'T':
              colour = "green";
              break;
            }
            var letter = document.createElement('span');
            letter.style.color = colour;
            letter.appendChild(document.createTextNode(sequence.charAt(i)));
            container.appendChild(letter);
          }
        }

        function append_coloured_peptide_sequence(container, sequence) {
          var len = sequence.length;
          for (var i = 0; i &lt; len; i++) {
            var colour = "black";
            switch (sequence.charAt(i)) {
            case 'A':
            case 'C':
            case 'F':
            case 'I':
            case 'L':
            case 'V':
            case 'W':
            case 'M':
              colour = "blue";
              break;
            case 'N':
            case 'Q':
            case 'S':
            case 'T':
              colour = "green";
              break;
            case 'D':
            case 'E':
              colour = "magenta";
              break;
            case 'K':
            case 'R':
              colour = "red";
              break;
            case 'H':
              colour = "pink";
              break;
            case 'G':
              colour = "orange";
              break;
            case 'P':
              colour = "yellow";
              break;
            case 'Y':
              colour = "turquoise";
              break;
            }
            var letter = document.createElement('span');
            letter.style.color = colour;
            letter.appendChild(document.createTextNode(sequence.charAt(i)));
            container.appendChild(letter);
          }
        }

        /*
         * Finds the index of an item in a sorted array (a) that equals a key (k)
         * when compared using a comparator (c). If an exact match can not be found
         * then returns -(index+1), where index is the location that the item would be inserted.
         */
        function bsearch(a, c, k) {
          var low = 0;
          var high = a.length;
          while (low &lt; high) {
            var mid = low + Math.floor((high - low) /2);
            if (c(a[mid], k) &lt; 0) {
              low = mid + 1;
            } else {
              high = mid;
            }
          }
          if ((low &lt; a.length) &amp;&amp; (c(a[low], k) == 0)) {
            return low;
          } else {
            return -(low+1);
          }
        }

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Motif Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Motif(name, type, best_f, best_rc) {
          this.name = name;
          this.is_nucleotide = (type === "nucleotide");
          this.best_f = best_f;
          this.best_rc = best_rc;
          this.length = best_f.length;
        }
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Motif Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Seg Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Seg(start, segment) {
          this.start = parseInt(start);
          this.segment = segment;
        }

        function compare_seg_to_pos(seg, pos) {
          return seg.start - pos;
        }
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Seg Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Hit Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Hit(sequence_is_nucleotide, motif, pos, strand, pvalue, match) {
          //properties
          this.motif = motif;
          this.pos = parseInt(pos);
          this.width = 0;
          this.is_rc = (strand != "+");
          this.pvalue = pvalue;
          this.best = "";
          this.match = "";
          //functions
          this.find_overlap = Hit_find_overlap;
          //setup
          var seq;
          var m = match.replace(/ /g, "&nbsp;");
          if (this.is_rc) {
            seq = this.motif.best_rc;
          }
          else {
            seq = this.motif.best_f;
          }
          if (sequence_is_nucleotide == this.motif.is_nucleotide) {
            this.best = seq;
            this.match = m;
            this.width = motif.length;
          } else if (sequence_is_nucleotide) {
            this.width = motif.length * 3;
            for (var i = 0; i &lt; motif.length; i++) {
              this.best += seq.charAt(i);
              this.best += "..";
              this.match += m.charAt(i);
              this.match += "&nbsp;&nbsp;";
            }
          } else {
            throw "UNSUPPORTED_CONVERSION";
          }
        }

        function Hit_find_overlap(sequence_is_nucleotide, start, width) {
          if (this.pos &lt; start) {
            if ((this.pos + this.width) &gt; start) {
              return {hit: this, start: start, length: Math.min(width, (this.pos + this.width - start))};
            } else {
              return null;
            }
          } else if (this.pos &lt; (start + width)) {
            return {hit: this, start: this.pos, length: Math.min(this.width, (start + width - this.pos))};
          }
          return null;
        }

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Hit Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Overlapping Hit Iterator
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function OLHIterator(sequence, start, width, is_rc) {
          //properties
          this.sequence = sequence;
          this.start = start;
          this.width = width;
          this.is_rc = is_rc;
          this.both = (is_rc === undefined);
          this.index = 0;
          this.nextO = null;
          //methods
          this.next = OLHIterator_next;
          this.has_next = OLHIterator_has_next;
          //setup
          // find the first hit which overlaps the range
          for (; this.index &lt; this.sequence.hits.length; this.index++) {
            var hit = this.sequence.hits[this.index];
            if (!this.both &amp;&amp; this.is_rc != hit.is_rc) continue;
            if ((this.nextO = hit.find_overlap(this.sequence.is_nucleotide, this.start, this.width)) != null) break;
          }
        }
        
        function OLHIterator_next() {
          if (this.nextO == null) throw "NO_NEXT_ELEMENT";
          var current = this.nextO;
          this.nextO = null;
          for (this.index = this.index + 1; this.index &lt; this.sequence.hits.length; this.index++) {
            var hit = this.sequence.hits[this.index];
            if (!this.both &amp;&amp; this.is_rc != hit.is_rc) continue;
            this.nextO = hit.find_overlap(this.sequence.is_nucleotide, this.start, this.width);
            break;
          }
          return current;
        }
        
        function OLHIterator_has_next() {
          return (this.nextO != null);
        }

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Overlapping Hit Iterator
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // Start Sequence Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        function Sequence(seqid) {
          //properties
          this.length = parseInt(document.getElementById(seqid + "_len").value);
          this.cpvalue = document.getElementById(seqid + "_combined_pvalue").value;
          this.is_nucleotide = (document.getElementById(seqid + "_type").value === "nucleotide");
          this.segs = new Array(); //sorted list of segs
          this.hits = new Array(); //sorted list of hits
          //functions
          this.get_overlapping_hits = Sequence_get_overlapping_hits;
          this.append_seq = Sequence_append_seq;
          this.append_translation = Sequence_append_translation;
          this.append_hits = Sequence_append_hits;
          this.append_matches = Sequence_append_matches;
          this.append_labels = Sequence_append_labels;
          this.append_boundary = Sequence_append_boundary;
          //init
          //made this parser much more permissive as a new
          //version of libxml2 broke the translate call I was
          //using to remove whitespace characters
          var mysegs = document.getElementById(seqid + "_segs");
          var tokens = mysegs.value.split(/\s+/);
          var offsetnum = "";
          var seqchunk = "";
          for (var i = 0; i &lt; tokens.length; i++) {
            var token = tokens[i];
            if (token == "") continue;
            if (token.match(/\d+/)) {
              if (offsetnum != "" &amp;&amp; seqchunk != "") {
                this.segs.push(new Seg(offsetnum, seqchunk));
              }
              offsetnum = parseInt(token, 10);
              seqchunk = "";
              continue;
            }
            seqchunk += token;
          }
          if (offsetnum != "" &amp;&amp; seqchunk != "") {
            this.segs.push(new Seg(offsetnum, seqchunk));
          }
          var myhits = document.getElementById(seqid + "_hits");
          lines = myhits.value.split(/\n/);
          for (var i in lines) {
            //exclude inherited properties and undefined properties
            if (!lines.hasOwnProperty(i) || lines[i] === undefined) continue;
            
            var line = lines[i];
            var chunks = line.split(/\t/);
            if (chunks.length != 5) continue;
            var pos = chunks[0];
            var motif = motifs[chunks[1]];
            var strand = chunks[2];
            var pvalue = chunks[3];
            var match = chunks[4];
            this.hits.push(new Hit(this.is_nucleotide, motif, pos, strand, pvalue, match));
          }
        }

        function Sequence_get_overlapping_hits(start, width, is_rc) {
          return new OLHIterator(this, start, width, is_rc);
        }

        function Sequence_append_seq(container, start, width, is_rc) {
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          // find where a seg starting at start would be
          var i = bsearch(this.segs, compare_seg_to_pos, start);
          var seg;
          var sequence = "";
          if (i &lt; -1) {
            //possible partial segment, need to handle first
            i = -(i + 1);
            seg = this.segs[i-1];
            if ((seg.start + seg.segment.length) &gt; start) {
              var seg_offset = start - seg.start;
              var seg_width = Math.min(width, seg.segment.length - seg_offset);
              sequence += seg.segment.substring(seg_offset, seg_offset + seg_width);
            }
          } else if (i == -1) {
            //gap, with following segment, normal state at start of iteration
            i = 0;
          } 
          for (;i &lt; this.segs.length; i++) {
            seg = this.segs[i];
            var gap_width = Math.min(width - sequence.length, seg.start - (start + sequence.length));
            for (; gap_width &gt; 0; gap_width--) sequence += '-';
            var seg_width = Math.min(width - sequence.length, seg.segment.length);
            if (seg_width == 0) break;
            sequence += seg.segment.substring(0, seg_width);
          }
          while (sequence.length &lt; width) sequence += '-';
          // calculate which parts are aligned with hits and output them in colour
          var pos = start;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var from, to;
            if (o.start &gt; pos) {
              from = pos - start;
              to = o.start - start;
              var subseq = sequence.substring(from, to);
              var greytxt = document.createElement('span');
              greytxt.style.color = 'grey';
              greytxt.appendChild(document.createTextNode(subseq));
              mycontainer.appendChild(greytxt);
              pos = o.start;
            }
            from = pos - start;
            to = from + o.length;
            var motifseq = sequence.substring(from, to);
            if (this.is_nucleotide) {
              append_coloured_nucleotide_sequence(mycontainer, motifseq);
            } else {
              append_coloured_peptide_sequence(mycontainer, motifseq);
            }
            pos += o.length;
          }
          if (pos &lt; (start + width)) {
            var greytxt = document.createElement('span');
            greytxt.style.color = 'grey';
            greytxt.appendChild(document.createTextNode(sequence.substring(pos - start)));
            mycontainer.appendChild(greytxt);
          }
          container.appendChild(mycontainer);
        }

        function Sequence_append_translation(container, start, width, is_rc) {
          if (start &gt; this.length) {
            alert("start: " + start + " length: " + this.length);
            throw "INDEX_OUT_OF_BOUNDS";
          }
          if ((start + width - 1) &gt; this.length) {
            alert("start: " + start + " width: " + width + " length: " + this.length);
            throw "RANGE_OUT_OF_BOUNDS";
          }
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          // calculate which parts are aligned with hits and output them in colour
          var pos = start;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var space = "";
            var from, to;
            while (o.start &gt; pos) {
              space += "&nbsp;";
              ++pos;
            }
            if (space.length &gt; 0) {
              var spacer = document.createElement('span');
              spacer.appendChild(document.createTextNode(space));
              mycontainer.appendChild(spacer);
            }
            from = o.start - o.hit.pos;
            to = from + o.length;
            if (o.hit.motif.is_nucleotide) {
              append_coloured_nucleotide_sequence(mycontainer, motifseq);
            } else {
              append_coloured_peptide_sequence(mycontainer, motifseq);
            }
            pos += o.length;
          }

          container.appendChild(mycontainer);
        }

        function Sequence_append_matches(container, start, width, is_rc) {
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          var pos = start;
          var text = "";
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var space = "";
            var from, to;
            while (o.start &gt; pos) {
              text += "&nbsp;";
              ++pos;
            }
            from = o.start - o.hit.pos;
            to = from + o.length;
            var motifseq = o.hit.match.substring(from, to);
            text += motifseq
            pos += o.length;
          }
          mycontainer.appendChild(document.createTextNode(text));
          container.appendChild(mycontainer);
        }

        function Sequence_append_hits(container, start, width, is_rc) {
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          // calculate which parts are aligned with hits and output them in colour
          var pos = start;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var space = "";
            var from, to;
            while (o.start &gt; pos) {
              space += "&nbsp;";
              ++pos;
            }
            if (space.length &gt; 0) {
              var spacer = document.createElement('span');
              spacer.appendChild(document.createTextNode(space));
              mycontainer.appendChild(spacer);
            }
            from = o.start - o.hit.pos;
            to = from + o.length;
            var motifseq = o.hit.best.substring(from, to);
            if (o.hit.motif.is_nucleotide) {
              append_coloured_nucleotide_sequence(mycontainer, motifseq);
            } else {
              append_coloured_peptide_sequence(mycontainer, motifseq);
            }
            pos += o.length;
          }

          container.appendChild(mycontainer);
        }

        function Sequence_append_labels(container, start, width, is_rc) {
          //create a table (ye gods...)
          var oTable = document.createElement("table");
          var oRow = oTable.insertRow(oTable.rows.length);

          // calculate where to put the labels
          var pos = start;
          var cellindex = 0;
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var motif_center = Math.floor(o.start + o.length / 2);
            var spacer = "";
            while (pos &lt; motif_center) {
              spacer += "&nbsp;";
              pos++;
            }
            var cell1 = oRow.insertCell(cellindex++);
            cell1.appendChild(document.createTextNode(spacer));
            //add one for the div
            pos++;
            var cell2 = oRow.insertCell(cellindex++);
            //create the div that holds the label divs
            var div_container = document.createElement('div');
            div_container.className = "label_container";
            cell2.appendChild(div_container);
            //create the top label
            var top_label = "";
            <xsl:variable name="db_dna" select="/mcast/alphabet/text() = 'nucleotide'"/>
            <xsl:variable name="has_strand" select="$db_dna"/>
            <xsl:if test="$has_strand">
              top_label += "(";
              if (o.hit.is_rc) {
                top_label += "-";
              } else {
                top_label += "+";
              }
              top_label += ") ";
            </xsl:if>
            if (o.hit.motif.name.match(/^\d+$/)) {
              top_label += "Motif " + o.hit.motif.name;
            } else {
              top_label += o.hit.motif.name;
            }
            var div_top_label = document.createElement('div');
            div_top_label.className = "sequence_label sequence_label_top";
            div_top_label.style.width = "" + top_label.length + "em";
            div_top_label.style.left = "-" + (top_label.length / 2) + "em";
            div_top_label.appendChild(document.createTextNode(top_label));
            //create the bottom label
            var bottom_label = o.hit.pvalue;
            var div_bottom_label = document.createElement('div');
            div_bottom_label.className = "sequence_label sequence_label_bottom";
            div_bottom_label.style.width = "" + bottom_label.length + "em";
            div_bottom_label.style.left = "-" + (bottom_label.length / 2) + "em";
            div_bottom_label.appendChild(document.createTextNode(bottom_label));
            //add the label divs to the positioned container
            div_container.appendChild(div_top_label);
            div_container.appendChild(div_bottom_label);
          }
          container.appendChild(oTable);

        }

        function Sequence_append_boundary(container, start, width, is_rc) {
          //make a sub container to put the sequence in
          var mycontainer = document.createElement('span');
          var pos = start;
          var text = "";
          var iter = this.get_overlapping_hits(start, width, is_rc);
          while (iter.has_next()) {
            var o = iter.next();
            var end;
            while (o.start &gt; pos) {
              text += "&nbsp;";
              ++pos;
            }
            if (o.start == o.hit.pos) {
              text += "\\";
              ++pos;
            }
            end = o.start + o.length - 1;
            while (end &gt; pos) {
              text += "_";
              ++pos;
            }
            if (end == (o.hit.pos + o.hit.width -1)) {
              text += "/"
            } else {
              text += "_"
            }
            ++pos;
          }
          mycontainer.appendChild(document.createTextNode(text));
          container.appendChild(mycontainer);
        }
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        // End Sequence Object
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      </script>
      <style type="text/css">
        <xsl:call-template name="meme.css" />
        .more_arrow {
          font-family:Arial,Serif;
          font-size: larger;
          font-weight: bold;
          text-decoration:none; 
        }
        div.infobox {
          background-color:#ddddff;
          margin-top: 50px;
          margin-bottom: 1em;
        }
        .sequence {font-family:Monospace;}
        .sequence_labels {position:relative; width:100%; height:2.5em; 
            padding:0px; margin:0px;}
        .sequence_label {font-family:Serif; position:absolute; z-index:2; 
            height:1em; text-align:center; vertical-align:middle;}
        .sequence_label_top {top:0px;}
        .sequence_label_bottom {top:1.25em;}
        .inlineTitle {display:inline;}
        .block_needle {position:absolute; z-index:4; height:30px; width:1px; 
            top:-2px; background-color:gray;}
        .block_handle {position:absolute; z-index:5; height:1.1em; width:7em; 
            top:30px; left:-4em; text-align:center; vertical-align:middle;
            background-color: LightGrey; border:3px outset grey; 
            -moz-user-select: none; -khtml-user-select: none; user-select: none;}
        .block_needle.right {height: 50px;}
        .block_handle.right {top:50px;}
        .label_container {position:relative; width:100%; height:25px; padding:0px; margin: 0px;}

        table.padded-table td { padding:0px 10px; }
        table.padded-table th { padding:0px 5px; }
        td.tnum {text-align:right;}
        tr.highlight {background:#aaffaa;}
        td.dim {color: gray;}
        span.dim {color: gray;}
        .col_seq { text-align: left; width: <xsl:value-of select="$longest_seq_name*0.8"/>em}
        .col_start, .col_stop, .col_score, .col_pv, .col_ev, .col_qv { text-align:right; }
        .col_start {width: 8em}
        .col_stop {width: 8em}
        .col_score {width: 5em}
        .col_pv {width: 5em}
        .col_ev {width: 5em}
        .col_qv {width: 5em}
        .col_more {width: 1em}
        .col_extra {width: *}
        .col_bd {width: 100%}
      </style>
    </head>
  </xsl:template>

  <xsl:template name="html-body">
    <body onload="javascript:setup()" >
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

  <xsl:template name="top">
    <a name="top"/>
    <div class="pad1">
      <h1><img src="http://meme.nbcr.net/meme/doc/images/mcast-logo.png" 
               alt="Motif ClusterAlignment and Search Tool (MCAST)" />
      </h1>
      <p class="spaced">
        For further information on how to interpret these results or to get a 
        copy of the MEME software please access 
        <a href="http://meme.nbcr.net/">http://meme.nbcr.net</a>. 
      </p>
    </div>
  </xsl:template>

  <xsl:template name="navigation">
    <a name="navigation"/>
    <div class="pad2">
      <a class="jump" href="#inputs">Inputs</a>
      &nbsp;&nbsp;|&nbsp;&nbsp;
      <a class="jump" href="#results">Search Results</a>
      &nbsp;&nbsp;|&nbsp;&nbsp;
      <a class="jump" href="#program">Program information</a>
      &nbsp;&nbsp;|&nbsp;&nbsp;
      <a class="jump" href="#doc">Documentation</a>
    </div>    
  </xsl:template>

  <xsl:template name="header">
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

  <xsl:template name="inputs" >
    <xsl:call-template name="header">
      <xsl:with-param name="title" select="'Inputs'"/>
      <xsl:with-param name="self" select="'inputs'"/>
    </xsl:call-template>
    <div class="box">
      <a name="databases"/>
      <h4>Sequence Database <a href="#databases_doc" class="help"><div class="help"/></a></h4>
      <div class="pad">
        <p>
          <xsl:text>The sequence database</xsl:text>
          &quot;<xsl:value-of
            select="document('cisml.xml')/cis:cis-element-search/cis:parameters/cis:sequence-file/text()"
          />&quot;
          <xsl:text>was supplied to MCAST.</xsl:text>
        </p>
        <table class="padded-table" border="0" >
          <col />
          <col />
          <thead>
            <tr>
              <th>Sequence Count</th>
              <th>Residue Count</th>
            </tr>
          </thead>
          <tbody>
              <tr>
                <td class="tnum"><xsl:value-of select="/mcast/sequence-data/@num-sequences"/></td>
                <td class="tnum"><xsl:value-of select="/mcast/sequence-data/@num-residues"/></td>
              </tr>
          </tbody>
          <tfoot>
          </tfoot>
        </table>
      </div>
      <a name="motifs" />
      <h4>Motifs <a href="#motifs_doc" class="help"><div class="help"/></a></h4>
      <div class="pad">
        <p>The following motifs were supplied to MCAST from
        &quot;<xsl:value-of
          select="document('cisml.xml')/cis:cis-element-search/cis:parameters/cis:pattern-file/text()"
        />&quot;
        </p>
        <xsl:variable name="stranded" select="string-length(/mcast/motif[1]/@best-possible-rc-match)"/>
        <table class="padded-table" border="0" >
          <thead>
            <tr>
              <th>&nbsp;</th>
              <th>&nbsp;</th>
              <th>&nbsp;</th>
              <xsl:if test="$stranded">
                <th>&nbsp;</th>
              </xsl:if>
            </tr>
            <tr>
              <th>Motif</th>
              <th>Width</th>
              <th>Best possible match</th>
              <xsl:if test="$stranded">
                <th>Best possible RC match</th>
              </xsl:if>
            </tr>
          </thead>
          <tbody>
            <xsl:for-each select="/mcast/motif">
              <xsl:variable name="motif_a_num" select="position()" />
              <xsl:variable name="motif_a" select="@id" />
              <tr>
                <td><xsl:value-of select="@name" /></td>
                <td><xsl:value-of select="@width" /></td>
                <td class="sequence">
                  <xsl:call-template name="colour-sequence" >
                    <xsl:with-param name="seq" select="@best-possible-match" />
                  </xsl:call-template>
                </td>
                <xsl:if test="$stranded">
                  <td class="sequence">
                    <xsl:call-template name="colour-sequence" >
                      <xsl:with-param name="seq" select="@best-possible-rc-match" />
                    </xsl:call-template>
                 </td>
                </xsl:if>
              </tr>
            </xsl:for-each>
          </tbody>
        </table>
      </div>
    </div>
  </xsl:template>

  <xsl:template name="results">

    <xsl:call-template name="header">
      <xsl:with-param name="title" select="'Search Results'"/>
      <xsl:with-param name="self" select="'results'"/>
    </xsl:call-template>
    <div class="box">
      <h4>Top Scoring Matches <a href="#sequences_doc" class="help"><div class="help"/></a></h4>
      <div class="pad">
        <p>
          Each of the following
          <xsl:value-of 
           select="count(document('cisml.xml')/cis:cis-element-search/cis:multi-pattern-scan)"/> matches 
          has a <i>q</i>-value less than 
          <xsl:value-of
          select="document('cisml.xml')/cis:cis-element-search/cis:parameters/cis:sequence-pvalue-cutoff/text()"/>.
          <br />
          The motif matches shown have a position p-value less than 
          <xsl:value-of
          select="document('cisml.xml')/cis:cis-element-search/cis:parameters/cis:pattern-pvalue-cutoff/text()"/>.<br />
          <b>Click on the arrow</b> (&more;) next to the block diagram to view more information about a sequence.
        </p>
        <xsl:call-template name="mcast_legend"/>
        <table id="tbl_sequences" style="width:100%; table-layout:fixed;" border="0">
          <thead>
            <tr>
              <th class="col_more">&nbsp;</th>
              <th class="col_seq">Sequence</th>
              <th class="col_start">Start</th>
              <th class="col_stop">Stop</th>
              <th class="col_score">Score</th>
              <th class="col_pv"><i>p</i>-value</th>
              <th class="col_ev"><i>E</i>-value</th>
              <th class="col_qv"><i>q</i>-value</th>
              <th class="col_extra">&nbsp;</th>
              <th class="col_more">&nbsp;</th>
            </tr>
          </thead>
          <tbody>
            <xsl:for-each select="document('cisml.xml')/cis:cis-element-search/cis:multi-pattern-scan">
              <xsl:variable name="seq_fract" select="((mem:match/@stop - mem:match/@start + 1) div $max_seq_len) * 100" />
              <xsl:variable name="match_start" select="mem:match/@start" />
              <xsl:variable name="cluster_id" select="mem:match/@cluster-id" />
              <tr>
                <td class="col_more">&nbsp;</td>
                <td class="col_seq"><xsl:value-of select="./mem:match/@seq-name" /></td>
                <td class="col_start"><xsl:value-of select="./mem:match/@start" /></td>
                <td class="col_stop"><xsl:value-of select="./mem:match/@stop" /></td>
                <td class="col_score"><xsl:value-of select="@score" /></td>
                <td class="col_pv">
                  <xsl:choose>
                    <xsl:when test="@pvalue = 'nan'">
                      <xsl:text>--</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="@pvalue" />
                    </xsl:otherwise>
                  </xsl:choose>
                </td>
                <td class="col_ev">
                  <xsl:choose>
                    <xsl:when test="./mem:match/@evalue = 'nan'">
                      <xsl:text>--</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="./mem:match/@evalue" />
                    </xsl:otherwise>
                  </xsl:choose>
                </td>
                <td class="col_qv">
                  <xsl:choose>
                    <xsl:when test="./mem:match/@qvalue = 'nan'">
                      <xsl:text>--</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="./mem:match/@qvalue" />
                    </xsl:otherwise>
                  </xsl:choose>
                </td>
              </tr>
              <tr class="col_bd" id="{$cluster_id}_blocks">
                <td class="col_more"><a href="javascript:show_more('{./mem:match/@cluster-id}')" class="more_arrow" title="Toggle additional information">&more;</a></td>
                <td class="block_td" colspan="8">
                  <div class="block_container" id="{./mem:match/@cluster-id}_block_container" >
                    <xsl:if test="$seq_is_dna">
                      <div class="block_plus_sym"><xsl:text>+</xsl:text></div>
                      <div class="block_minus_sym"><xsl:text>-</xsl:text></div>
                    </xsl:if>
                    <div class="block_rule" style="width:{$seq_fract}%"></div>
                    <xsl:for-each select="cis:pattern/cis:scanned-sequence/cis:matched-element">
                      <xsl:for-each select="/mcast/motif">
                        <xsl:if test="@name=$motif">
                          <xsl:value-of select="position()"/>
                        </xsl:if>
                      </xsl:for-each>
                      <xsl:variable name="motif" select="../../@name" />
                      <xsl:variable name="motif_num">
                        <xsl:call-template name="motif_index">
                          <xsl:with-param name="motif_name" select="$motif"/>
                        </xsl:call-template>
                      </xsl:variable>
                      <xsl:variable name="strand">
                        <xsl:choose>
                          <xsl:when test="@stop &gt; @start">
                            <xsl:text>plus</xsl:text>
                          </xsl:when>
                          <xsl:otherwise>
                            <xsl:text>minus</xsl:text>
                          </xsl:otherwise>
                        </xsl:choose>
                      </xsl:variable>
                      <xsl:variable name="position">
                        <xsl:choose>
                          <xsl:when test="@stop &gt; @start">
                            <xsl:value-of select="@start"/>
                          </xsl:when>
                          <xsl:otherwise>
                            <xsl:value-of select="@stop"/>
                          </xsl:otherwise>
                        </xsl:choose>
                      </xsl:variable>
                      <xsl:variable name="width">
                        <xsl:choose>
                          <xsl:when test="@stop &gt; @start">
                            <xsl:value-of select="(@stop - @start + 1)"/>
                          </xsl:when>
                          <xsl:otherwise>
                            <xsl:value-of select="(@start - @stop + 1)"/>
                          </xsl:otherwise>
                        </xsl:choose>
                      </xsl:variable>
                      <xsl:call-template name="site">
                        <xsl:with-param name="diagram_len" select="$max_seq_len"/>
                        <xsl:with-param name="best_log_pvalue" select="$max_log_pvalue"/>
                        <xsl:with-param name="start_position" select="$match_start"/>
                        <xsl:with-param name="position" select="$position"/>
                        <xsl:with-param name="width" select="$width"/>
                        <xsl:with-param name="index" select="$motif_num"/>
                        <xsl:with-param name="strand" select="$strand"/>
                        <xsl:with-param name="pvalue" select="@pvalue"/>
                        <xsl:with-param name="name" select="$motif"/>
                      </xsl:call-template>
                    </xsl:for-each>
                  </div>
                </td>
              </tr>
            </xsl:for-each>
          </tbody>
        </table>
      </div>
    </div>
  </xsl:template>

  <xsl:template name="motif_index">
    <xsl:param name="motif_name"/>
    <xsl:for-each select="document('mcast.xml')/mcast/motif">
      <xsl:if test="@name=$motif_name">
        <xsl:value-of select="position()"/>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="mcast_legend">
    <div style="text-align:left">
      <xsl:for-each select="/mcast/motif">
        <xsl:call-template name="legend_motif">
          <xsl:with-param name="name" select="@name"/>
          <xsl:with-param name="index" select="position()"/>
        </xsl:call-template>
      </xsl:for-each>
    </div>
  </xsl:template>

  <xsl:template name="program">
    <a name="program"/>
    <div class="bar">
      <div style="text-align:right;"><a href="#top">Top</a></div>
      <div class="subsection">
        <a name="version"/>
        <h5>MCAST version</h5>
        <xsl:value-of select="/mcast/@version"/> (Release date: <xsl:value-of select="/mcast/@release"/>)
      </div>
      <div class="subsection">
        <a name="reference"/>
        <h5>Reference</h5>
        Timothy Bailey and William Stafford Noble,<br /> 
        &quot;Searching for statistically significant regulatory modules&quot;, 
        <i>Bioinformatics (Proceedings of the European Conference on Computational Biology)</i>,
        19(Suppl. 2):ii16-ii25, 2003.<br/>
      </div>
      <div class="subsection">
        <a name="command" />
        <h5>Command line summary</h5>
        <textarea rows="1" style="width:100%;" readonly="readonly">
          <xsl:value-of select="/mcast/command-line"/>
        </textarea>
        <br />
        <xsl:text>Background letter frequencies (from </xsl:text>
        <xsl:value-of select="/mcast/background/@source" />
        <xsl:text>):</xsl:text><br/>
        <div style="margin-left:25px;">
          <xsl:for-each select="/mcast/background/value">
            <xsl:value-of select="@letter"/><xsl:text>:&nbsp;</xsl:text><xsl:value-of select="format-number(text(), '0.000')" />
            <xsl:if test="position() != last()"><xsl:text>&nbsp;&nbsp; </xsl:text></xsl:if>            
          </xsl:for-each>
        </div>
        <br />
      </div>      
      <a href="javascript:showHidden('model')" id="model_activator">show model parameters...</a>
      <div class="subsection" id="model_data" style="display:none;">
      </div>
      <a href="javascript:hideShown('model')" style="display:none;" id="model_deactivator">hide model parameters...</a>
    </div>
  </xsl:template>

  <xsl:template name="documentation">
    <span class="explain">
    <xsl:call-template name="header">
      <xsl:with-param name="title" select="'Explanation of MCAST Results'"/>
      <xsl:with-param name="self" select="'doc'"/>
    </xsl:call-template>
      <div class="box">
        <h4>The MCAST results consist of</h4>
        <ul>
          <li>The <a href="#input_motifs_doc"><b>inputs</b></a> to MCAST including:
            <ol>
              <li>The <a href="#databases_doc"><b>sequence database</b></a> showing the sequence 
                and residue counts. [<a href="#databases">View</a>]</li>
              <li>The <a href="#motifs_doc"><b>motifs</b></a> showing the name, width,
              and best scoring match [<a href="#motifs">View</a>]</li> 
            </ol>
          </li>
          <li>The <a href="#sequences_doc"><b>search results</b></a> showing top scoring sequences with 
            tiling of all of the motifs matches shown for each of the sequences. [<a href="#results">View</a>]
          </li>
          <li>The <b>program</b> details including:
            <ol>
              <li>The <b>version</b> of MCAST and the date it was released. [<a href="#version">View</a>]</li>
              <li>The <b>reference</b> to cite if you use MCAST in your research. [<a href="#reference">View</a>]</li>
              <li>The <b>command line summary</b> detailing the parameters with which you ran MCAST. [<a href="#command">View</a>]</li>
            </ol>
          </li>
          <li>This <b>explanation</b> of how to interpret MCAST results.</li>
        </ul>
        <a name="input_motifs_doc"/>
        <h4>Inputs</h4>
        <p>MCAST received the following inputs.</p>
        <a name="databases_doc"/>
        <h5>Sequence Databases</h5>
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
        <a name="motifs_doc"/>
        <h5>Motifs</h5>
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
          </dl>
        </div>
        <a name="sequences_doc"/>
        <h4>Search Results</h4>
        <p>MCAST provides the following motif search results.</p>
        <h5>Top Scoring Sequences</h5>
        <div class="doc">
          <p>
            This table summarises the top scoring sequences.
            The sequences are sorted by the <a href="#evalue_doc">Sequence 
            <i>q</i>-value</a> from most to least significant.
          </p>
          <dl>
            <dt>Sequence</dt>
            <dd>The name of the sequence. This maybe be linked to search a sequence database for the sequence name.</dd>
            <dt>Start</dt>
            <dd>
              The position of the start of the match in 1-based coordinates, starting at the
              first position in the sequence.
            </dd>
            <dt>Stop</dt>
            <dd>
              The position of the end of the match in 1-based coordinates, starting at the
              first position in the sequence.
            </dd>
            <dt>Score</dt>
            <dd>
              The MCAST score for the match.
            </dd>
            <dt><i>p</i>-value</dt>
            <dd>
              The <a href="#pvalue_doc"><i>p</i>-value</a> of the match score.
            </dd>
            <dt><i>E</i>-value</dt>
            <dd>
              The <a href="#evalue_doc"><i>E</i>-value</a> of the match score.
            </dd>
            <dt><i>q</i>-value</dt>
            <dd>
              The <a href="#qvalue_doc"><i>q</i>-value</a> of the match score.
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
        <a name="additional_seq_doc"/>
      </div>
    </span>
  </xsl:template>

  <xsl:template name="data" >
    <xsl:text>&newline;</xsl:text>
    <xsl:comment >This data is used by the javascript to create the detailed views</xsl:comment><xsl:text>&newline;</xsl:text>
    <form>
      <xsl:variable name="type">
        <xsl:value-of select="/mcast/alphabet/text()"/>
      </xsl:variable>
      <xsl:for-each select="document('cisml.xml')/cis:cis-element-search/cis:multi-pattern-scan/mem:match">
        <xsl:variable name="segs">
          <xsl:text>&newline;</xsl:text>
          <xsl:value-of select="@start"/>
          <xsl:text>&newline;</xsl:text>
          <xsl:value-of select="./text()"/>
        </xsl:variable>
        <xsl:variable name="hits">
          <xsl:text>&newline;</xsl:text>
          <xsl:for-each select="../cis:pattern/cis:scanned-sequence/cis:matched-element"> 
            <xsl:choose>
              <xsl:when test="@start &lt; @stop">
                <xsl:value-of select="@start" /><xsl:text>&tab;</xsl:text> 
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="@stop" /><xsl:text>&tab;</xsl:text> 
              </xsl:otherwise>
            </xsl:choose>
            <xsl:value-of select="../../@name" /><xsl:text>&tab;</xsl:text>
            <xsl:choose>
              <xsl:when test="@start &lt; @stop">
                <xsl:text>+&tab;</xsl:text>
              </xsl:when>
              <xsl:otherwise>
                <xsl:text>-&tab;</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:value-of select="@pvalue"/><xsl:text>&tab;</xsl:text> 
            <xsl:value-of select="./cis:sequence/text()"/><xsl:text>&newline;</xsl:text>
          </xsl:for-each>
        </xsl:variable>
        <input type="hidden" id="{@cluster-id}_len" value="{@stop - @start + 1}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@cluster-id}_combined_pvalue" value="{@pvalue}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@cluster-id}_type" value="{$type}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@cluster-id}_segs" value="{$segs}" /><xsl:text>&newline;</xsl:text>
        <input type="hidden" id="{@cluster-id}_hits" value="{$hits}" /><xsl:text>&newline;</xsl:text>
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
    <xsl:param name="isnuc" select="/mcast/alphabet/text()= 'nucleotide'" />
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
