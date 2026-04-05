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

## very far from initial update

    this project came and went and i worked on it and i didnt. its soul crushing to work on crud apps in your free time.
    but i did it, and its coming along.
    templated query fn makes way more sense, and there are some bad auth ideas i had that i need to fix.
    overall, not much has changed -- cpp backend w crow, frontend is just simply html/css/js
    i think i will use git as a middle man to track state somehow :think:
    otherwise i will end up doing a lot of expensive diff ops
    next on mind, populating tracks table, now playing player view, actual diff suggestions
    also need to pull auth out into middleware and make it actually work
