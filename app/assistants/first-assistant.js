function FirstAssistant() {
	/* this is the creator function for your scene assistant object. It will be passed all the
	   additional parameters (after the scene name) that were passed to pushScene. The reference
	   to the scene controller (this.controller) has not be established yet, so any initialization
	   that needs the scene controller should be done in the setup function below. */
}

FirstAssistant.prototype.filterFunction = function(filterString, listWidget, offset, count) {

};

FirstAssistant.prototype.fillFileList = function(event_or_path, path) {
	if (path === undefined || path === null) {
		if (event_or_path + "" === event_or_path) {
			path = event_or_path;
		} else {
			path = "/media/internal";
		}
	}
	$('fileListTitleId').update(path);

	var split_and_convert = function(str, img) {
		return str.split("/").collect(function(el) {
			return {data: el, icon: img};
		});
	};

	// Testing.
	// $('fileListId').mojo.noticeAddedItems(0, [{data: "item", icon: "file"}, {data: "item2", icon: "folder"}] );
	$('fileListId').mojo.noticeRemovedItems(0, $('fileListId').mojo.getLength());
	var files = split_and_convert($('premplayer_plugin').list_files(path), "file");
	$('fileListId').mojo.noticeAddedItems(0, files);
	var directories = split_and_convert($('premplayer_plugin').list_directories(path), "folder");
	$('fileListId').mojo.noticeAddedItems(0, directories);
};

writeObj = function(obj, message) {
  if (!message) { message = obj; }
  var details = "*****************" + "\n" + message + "\n";
  var fieldContents;
  for (var field in obj) {
    fieldContents = obj[field];
    if (typeof(fieldContents) == "function") {
      fieldContents = "(function)";
    }
    details += "  " + field + ": " + fieldContents + "\n";
  }
  console.log(details);
};

FirstAssistant.prototype.handleListTap = function(event) {
	if (event.item.icon === "folder") {
		var current_path = $('fileListTitleId').innerHTML;
		if (event.item.data === ".") {
			// Just refresh
		} else if (event.item.data === "..") {
			// Move up
			current_path = current_path.slice(0, current_path.lastIndexOf("/"));
		} else {
			current_path = current_path + "/" + event.item.data;
		}
		this.fillFileList(current_path);
	} else if (event.item.icon === "file") {
		writeObj(event.item, "TODO: Add this to list");
	}
};


FirstAssistant.prototype.setup = function() {
	/* this function is for setup tasks that have to happen when the scene is first created */

	/* use Mojo.View.render to render view templates and add them to the scene, if needed */

	/* setup widgets here */
	this.controller.setupWidget("fileListId",
		this.attributes = {
			itemTemplate: "first/static-file-list-entry",
			listTemplate: "first/static-file-list-container",
			swipeToDelete: false,
			reorderable: false,
			filterFunction: this.filterFunction.bind(this),
			emptyTemplate:"filterlist/emptylist"
		},
		this.model = {
			listTitle: "/media/internal",
			disabled: false
		}
	);
	this.controller.listen(this.controller.get("fileListId"), Mojo.Event.listTap, this.handleListTap.bind(this));

	/* update the app info using values from our app */
	this.controller.get("app-title").update(Mojo.appInfo.title);
	this.controller.get("app-id").update(Mojo.appInfo.id);
  this.controller.get("app-version").update(Mojo.appInfo.version);

	Mojo.Event.listen(this.controller.get("buttonId"), Mojo.Event.tap, this.fillFileList);
	/* add event handlers to listen to events from widgets */
};

FirstAssistant.prototype.activate = function(event) {
	/* put in event handlers here that should only be in effect when this scene is active. For
	   example, key handlers that are observing the document */
};

FirstAssistant.prototype.deactivate = function(event) {
	/* remove any event handlers you added in activate and do any other cleanup that should happen before
	   this scene is popped or another scene is pushed on top */
};

FirstAssistant.prototype.cleanup = function(event) {
	/* this function should do any cleanup needed before the scene is destroyed as
	   a result of being popped off the scene stack */
	$('premplayer_plugin').kill();
};
