PlayAudio = {
	clip_name = "",
	does_loop = 0,
	channel = 0,
	OnStart = function(self)

		Audio.Play(self.channel, self.clip_name, self.does_loop)
	end
}