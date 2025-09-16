#rsync -v --info=progress2 --size-only /home/freeman/repo/generator/data/* data_train/
rsync -v --info=progress2 --size-only freeman@192.168.122.86:/home/freeman/repo/generator/data/* data_train/
rsync -v --info=progress2 --size-only freeman@192.168.122.87:/home/freeman/repo/generator/data/* data_train/
rsync -v --info=progress2 --size-only freeman@192.168.122.88:/home/freeman/repo/generator/data/* data_train/
rsync -v --info=progress2 --size-only freeman@192.168.122.89:/home/freeman/repo/generator/data/* data_train/
rsync -v --info=progress2 --size-only freeman@192.168.122.90:/home/freeman/repo/generator/data/* data_train/
#rsync -v --info=progress2 --size-only freeman@192.168.122.91:/home/freeman/repo/generator/data/* data_train/
date
ls data_train/ | wc -l
