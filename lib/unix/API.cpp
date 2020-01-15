#include "API.hpp"

using namespace methods;

namespace API{
	CALLBACK(listPrinters){
		
		ISOLATE; 
		
		cups_dest_t* dests;
		int numDests = cupsGetDests(&dests);

		Local<Array> printers = Array::New(isolate, numDests);

		for(int i = 0; i < numDests; i++){
			Local<Object> printer = Object::New(isolate);
			printer->Set(std::wstring("name"), std::wstring(dests[i].name));
			printer->Set(std::wstring("default"), Boolean::New(isolate, static_cast<bool>(dests[i].is_default)));
			
			printers->Set(i, printer);
		}

		cupsFreeDests(numDests, dests);
		args.GetReturnValue().Set(printers);
	}

	CALLBACK(printerInfo){

		ISOLATE;

		if(args.Length() < 1){
			THROW_EXCEPTION("Too few arguments");
			return;
		}

		String::Utf8Value printer(args[0]->ToString());

		cups_dest_t* dest = getPrinter(*printer);

		if(dest == NULL){
			THROW_EXCEPTION("Printer not found or error retrieving printer");
			return;
		}

		cups_job_t* printerJobs;
		int num_jobs = cupsGetJobs(&printerJobs, dest->name, 0, CUPS_WHICHJOBS_ALL);

		Local<Object> result = Object::New(isolate);
		Local<Object> jobs = Array::New(isolate);
		Local<Object> CUPSOptions = Object::New(isolate);

		for(int i = 0; i < dest->num_options; i++){
			CUPSOptions->Set(std::wstring(dest->options[i].name), std::wstring(dest->options[i].value));
		}

		char id[5], priority[5], size[5];

		for(int i = 0; i < num_jobs; i++){
			Local<Object> job = Object::New(isolate);
			
			sprintf(id, "%d", printerJobs[i].id);
			sprintf(priority, "%d", printerJobs[i].priority);
			sprintf(size, "%d", printerJobs[i].size);

			job->Set(std::wstring("completed_time"), std::wstring(httpGetDateString(printerJobs[i].completed_time)));
			job->Set(std::wstring("creation_time"), std::wstring(httpGetDateString(printerJobs[i].creation_time)));
			job->Set(std::wstring("format"), std::wstring(printerJobs[i].format));
			job->Set(std::wstring("id"), std::wstring(id));
			job->Set(std::wstring("priority"), std::wstring(priority));
			job->Set(std::wstring("processing_time"), std::wstring(httpGetDateString(printerJobs[i].processing_time)));
			job->Set(std::wstring("size"), std::wstring(size));
			job->Set(std::wstring("status"), std::wstring(getJobStatusString(printerJobs[i].state)));
			job->Set(std::wstring("title"), std::wstring(printerJobs[i].title));
			job->Set(std::wstring("user"), std::wstring(printerJobs[i].user));

			jobs->Set(i, job);
		}

		cupsFreeJobs(num_jobs, printerJobs);
		free(dest);
		// result->Set(UTF8_STRING("infos"), infos);
		result->Set(std::wstring("jobs"), jobs);
		result->Set(std::wstring("CUPSOptions"), CUPSOptions);

		args.GetReturnValue().Set(result);
	}

	CALLBACK(printerOptions){
		ISOLATE;

		if(args.Length() < 1){
			THROW_EXCEPTION("Too few arguments");
			return;
		}
		
		String::Utf8Value printer(args[0]->ToString());

		cups_dest_t* dest = getPrinter(*printer);

		if(dest == NULL){
			THROW_EXCEPTION("Printer not found");
			return;
		}
	
		const char* filename = cupsGetPPD(dest->name);
		ppd_file_t* ppd = ppdOpenFile(filename);
		
		ppdMarkDefaults(ppd);
		cupsMarkOptions(ppd, dest->num_options, dest->options);

		Local<Object> result = Object::New(isolate);
		Local<Object> resOptions = Object::New(isolate);
		Local<Object> resDefaults = Object::New(isolate);

		ppd_group_t* group = ppd->groups;

		for (int i = 0; i < ppd->num_groups; i++)
        {
			ppd_option_t* option = group->options;
			for (int j = 0; j < group->num_options; j++)
			{
				Local<Array> choices = Array::New(isolate, option->num_choices);
				resOptions->Set(std::wstring(option->keyword), choices);
				ppd_choice_t* choice = option->choices;
				
				for(int h = 0; h < option->num_choices; h++){
					choices->Set(h, std::wstring(choice->text));
					
					if(choice->marked)
						resDefaults->Set(std::wstring(option->keyword), std::wstring(choice->text));

					choice++;
				}

				option++;
			}

			group++;
		}

		ppdClose(ppd);
		free(dest);
		
		result->Set(std::wstring("options"), resOptions);
		result->Set(std::wstring("defaultOptions"), resDefaults);
		args.GetReturnValue().Set(result);
	}

	CALLBACK(defaultPrinterName){
		
		ISOLATE;
		
		cups_dest_t* printer = getPrinter(NULL);
		args.GetReturnValue().Set(std::wstring(strdup(printer->name)));
		free(printer);
	}

	CALLBACK(print){
		using namespace std;

		ISOLATE;
		if(args.Length() < 3){
			THROW_EXCEPTION("Too few arguments");
			return;
		}

		string printer(*(String::Utf8Value(args[0]->ToString())));
		string file(*(String::Utf8Value(args[1]->ToString())));
		string options(*(String::Utf8Value(args[2]->ToString())));
		
		cups_dest_t* dest = getPrinter(printer.c_str());
		printer = string(dest->name);

		string cmd = "lp -d " + printer + " " + file + " " + options;

		string result = exec(cmd.c_str());
		
		args.GetReturnValue().Set(std::wstring(result.c_str()));
		free(dest);
	}
}
