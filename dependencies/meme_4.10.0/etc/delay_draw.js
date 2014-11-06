      // 
      // return true if any part of the passed element is visible in the viewport
      //
      function element_in_viewport(elem) {
        var rect = elem.getBoundingClientRect();
        return (
            rect.top < (window.innerHeight || document.body.clientHeight) &&
            rect.bottom > 0 &&
            rect.left < (window.innerWidth || document.body.clientWidth) &&
            rect.right > 0
            );
      }

      //
      // Functions to delay a drawing task until it is required or it would not lag the display to do so
      //

      // a list of items still to be drawn
      var drawable_list = [];
      // the delay between drawing objects that are not currently visible
      var draw_delay = 1;
      // the delay after a user interaction
      var user_delay = 300;
      // the delay after a user has stopped scrolling and is viewing the stuff drawn on the current page
      var stop_delay = 300;
      // the timer handle; allows resetting of the timer after user interactions
      var draw_timer = null;

      //
      // Drawable
      //
      // elem - a page element which defines the position on the page that drawing is to be done
      // task - an object with the method run which takes care of painting the object
      //
      var Drawable = function(elem, task) {
        this.elem = elem;
        this.task = task;
      }

      //
      // Drawable.is_visible
      //
      // Determines if the element is visible in the viewport
      //
      Drawable.prototype.is_visible = function() {
        return element_in_viewport(this.elem);
      }

      //
      // Drawable.run
      //
      // Run the task held by the drawable
      Drawable.prototype.run = function() {
        if (this.task) this.task.run();
        this.task = null;
      }

      //
      // Drawable.run
      //
      // Run the task iff visible
      // returns true if the task ran or has already run
      Drawable.prototype.run_visible = function() {
        if (this.task) {
          if (element_in_viewport(this.elem)) {
            this.task.run();
            this.task = null;
            return true;
          }
          return false;
        } else {
          return true;
        }
      }

      //
      // draw_on_screen
      //
      // Checks each drawable object and draws those on screen.
      //
      function draw_on_screen() {
        var found = false;
        for (var i = 0; i < drawable_list.length; i++) {
          if (drawable_list[i].run_visible()) {
            drawable_list.splice(i--, 1);
            found = true;
          }
        }
        return found;
      }

      //
      // process_draw_tasks
      //
      // Called on a delay to process the next avaliable
      // draw task.
      //
      function process_draw_tasks() {
        var delay = draw_delay;
        draw_timer = null;
        if (drawable_list.length == 0) return; //no more tasks
        if (draw_on_screen()) {
          delay = stop_delay; //give the user a chance to scroll
        } else {
          //get next task
          var drawable = drawable_list.shift();
          drawable.task.run();
        }
        //allow UI updates between tasks
        draw_timer = window.setTimeout("process_draw_tasks()", delay);
      }

      //
      // delayed_process_draw_tasks
      //
      // Call process_draw_tasks after a short delay.
      // The delay serves to group multiple redundant events.       
      // Should be set as event handler for onscroll and onresize.
      //
      function delayed_process_draw_tasks() {
        //reset the timer
        if (drawable_list.length > 0) { 
          if (draw_timer != null) clearTimeout(draw_timer);
          draw_timer = window.setTimeout("process_draw_tasks()", user_delay);
        }
      }

      //
      // add_draw_task
      //
      // Add a drawing task to be called immediately if it is
      // visible, or to be called on a delay to reduce stuttering
      // effect on the web browser.
      function add_draw_task(elem, task) {
        drawable = new Drawable(elem, task);
        if (drawable.is_visible()) {
          task.run();
        } else {
          drawable_list.push(drawable);
          //reset timer
          if (draw_timer != null) clearTimeout(draw_timer);
          draw_timer = window.setTimeout("process_draw_tasks()", user_delay);
        }
      }

