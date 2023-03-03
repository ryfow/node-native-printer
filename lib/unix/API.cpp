#include "API.hpp"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

using namespace methods;

namespace API{
  Local<v8::String> v8_str(Isolate* isolate, const char * param) {
    return v8::String::NewFromUtf8(isolate, param).ToLocalChecked();
  }

	CALLBACK(listPrinters){

		ISOLATE;

		cups_dest_t* dests;
		int numDests = cupsGetDests(&dests);

		Local<Array> printers = Array::New(isolate, numDests);
    Local<Context> ctx = isolate->GetCurrentContext();
		for(int i = 0; i < numDests; i++){
			Local<Object> printer = Object::New(isolate);
      //v8::Local<v8::String> result;
      //v8::MaybeLocal<v8::String> temp = String::NewFromUtf8(isolate, "name");
      //temp.ToLocal(&result);

      //v8::Local<v8::String> result2;
      //v8::MaybeLocal<v8::String> temp2 = String::NewFromUtf8(isolate, dests[i].name);
      //temp2.ToLocal(&result2);

			printer->Set(ctx, v8::String::NewFromUtf8(isolate, "name").ToLocalChecked(), v8::String::NewFromUtf8(isolate, dests[i].name).ToLocalChecked()).Check();

			printer->Set(ctx, v8::String::NewFromUtf8(isolate, "name").ToLocalChecked(), Boolean::New(isolate, static_cast<bool>(dests[i].is_default))).Check();

			printers->Set(ctx, i, printer).Check();
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

		//auto printer(args[0]->ToString(isolate->GetCurrentContext()).ToLocalChecked());
    String::Utf8Value utf8_value(isolate, args[0]);
		cups_dest_t* dest = getPrinter(string(*utf8_value).c_str());

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
			CUPSOptions->Set(isolate->GetCurrentContext(), v8_str(isolate, dest->options[i].name), v8_str(isolate, dest->options[i].value)).Check();
		}

		char id[5], priority[5], size[5];

		for(int i = 0; i < num_jobs; i++){
			Local<Object> job = Object::New(isolate);

			sprintf(id, "%d", printerJobs[i].id);
			sprintf(priority, "%d", printerJobs[i].priority);
			sprintf(size, "%d", printerJobs[i].size);

			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "completed_time"), v8_str(isolate, httpGetDateString(printerJobs[i].completed_time))).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "creation_time"), v8_str(isolate, httpGetDateString(printerJobs[i].creation_time))).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "format"), v8_str(isolate, printerJobs[i].format)).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "id"), v8_str(isolate, id)).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "priority"), v8_str(isolate, priority)).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "processing_time"), v8_str(isolate, httpGetDateString(printerJobs[i].processing_time))).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "size"), v8_str(isolate, size)).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "status"), v8_str(isolate, getJobStatusString(printerJobs[i].state))).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "title"), v8_str(isolate, printerJobs[i].title)).Check();
			job->Set(isolate->GetCurrentContext(), v8_str(isolate, "user"), v8_str(isolate, printerJobs[i].user)).Check();

			jobs->Set(isolate->GetCurrentContext(), i, job).Check();
		}

		cupsFreeJobs(num_jobs, printerJobs);
		free(dest);
		// result->Set(UTF8_STRING("infos"), infos);
		result->Set(isolate->GetCurrentContext(), v8_str(isolate, "jobs"), jobs).Check();
		result->Set(isolate->GetCurrentContext(), v8_str(isolate, "CUPSOptions"), CUPSOptions).Check();

		args.GetReturnValue().Set(result);
	}

	CALLBACK(printerOptions){
		ISOLATE;

		if(args.Length() < 1){
			THROW_EXCEPTION("Too few arguments");
			return;
		}

		String::Utf8Value printer(isolate, args[0]);

		cups_dest_t* dest = getPrinter(string(*printer).c_str());

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
				resOptions->Set(isolate->GetCurrentContext(), v8_str(isolate, option->keyword), choices).Check();
				ppd_choice_t* choice = option->choices;

				for(int h = 0; h < option->num_choices; h++){
					choices->Set(isolate->GetCurrentContext(), h, v8_str(isolate, choice->text)).Check();

					if(choice->marked)
						resDefaults->Set(isolate->GetCurrentContext(), v8_str(isolate, option->keyword), v8_str(isolate, choice->text)).Check();

					choice++;
				}

				option++;
			}

			group++;
		}

		ppdClose(ppd);
		free(dest);

		result->Set(isolate->GetCurrentContext(), v8_str(isolate, "options"), resOptions).Check();
		result->Set(isolate->GetCurrentContext(), v8_str(isolate, "defaultOptions"), resDefaults).Check();
		args.GetReturnValue().Set(result);
	}

	CALLBACK(defaultPrinterName){

		ISOLATE;

		cups_dest_t* printer = getPrinter(NULL);
		args.GetReturnValue().Set(v8_str(isolate, strdup(printer->name)));
		free(printer);
	}

	CALLBACK(print){
		using namespace std;

		ISOLATE;
		if(args.Length() < 3){
			THROW_EXCEPTION("Too few arguments");
			return;
		}


		string printer(*(String::Utf8Value(isolate, args[0])));
		string file(*(String::Utf8Value(isolate, args[1])));
		string options(*(String::Utf8Value(isolate, args[2])));

		cups_dest_t* dest = getPrinter(printer.c_str());
		printer = string(dest->name);

		string cmd = "lp -d " + printer + " " + file + " " + options;

		string result = exec(cmd.c_str());

		args.GetReturnValue().Set(v8_str(isolate, result.c_str()));
		free(dest);
	}
}
