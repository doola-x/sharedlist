# sharedlist
	a cross platform playlist manager. ever wanted to create a playlist with a friend but you have different music subscriptions? or they don't even have one at all?

## initial plan
	create an apple music/spotify integration.
	if you sign into your spotify/apple music account (why/how, what will i need from this), 
	you can manage a playlist from the website itself, (website?)
	(these changes will have to be queued and made on behalf of each user)
	or if you create changes on your service provider they will be merged back into the main playlist when approved by the other users.
	User Object
	Playlist Object
	Song Object
	PlaylistAction Object
	Comment Object
	Approval Object

## initial thoughts

	if i really want this to be usable this should be an iOS native app as well. but a website will do for now
	i imagine auth is going to be 3rd party idp service (oauth with spotify/apple) but im unsure about how to make api requests on behalf of the user.
	i think ill write a svelte frontend and a go/rust backend... c++/c seems insane (but maybe doable?)


## first night conclusions

	i think ill actually write a c backend as insane as that sounds. 
	frontend will just be html/css/js with scripts that call a series of executables on the backend, 
	and ill use sqlite3 as a db since its lightweight and has built in support/library for c.
	first and potentially most formidable is some sort of auth handler. this also needs to help with spotify/apple music logins....
	lawd
